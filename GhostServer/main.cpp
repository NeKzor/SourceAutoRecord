#include <SFML/Network.hpp>
#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

struct QAngle {
    float x;
    float y;
    float z;
};

enum HEADER {
    NONE,
    PING,
    CONNECT,
    UPDATE,
    DISCONNECT,
    STOP_SERVER
};

struct DataGhost {
    QAngle position;
    QAngle view_angle;
    std::string currentMap;
};

struct NetworkDataPlayer {
    HEADER header;
    std::string name;
    std::string ip;
    unsigned short port;
    DataGhost dataGhost;
    std::string message;
};

//DataGhost

sf::Packet& operator>>(sf::Packet& packet, QAngle& angle)
{
    return packet >> angle.x >> angle.y >> angle.z;
}

sf::Packet& operator>>(sf::Packet& packet, DataGhost& dataGhost)
{
    return packet >> dataGhost.position >> dataGhost.view_angle >> dataGhost.currentMap;
}

sf::Packet& operator<<(sf::Packet& packet, const QAngle& angle)
{
    return packet << angle.x << angle.y << angle.z;
}

sf::Packet& operator<<(sf::Packet& packet, const DataGhost& dataGhost)
{
    return packet << dataGhost.position << dataGhost.view_angle << dataGhost.currentMap;
}

//COMMAND

sf::Packet& operator>>(sf::Packet& packet, HEADER& header)
{
    sf::Uint8 tmp;
    packet >> tmp;
    header = static_cast<HEADER>(tmp);
    return packet;
}

sf::Packet& operator<<(sf::Packet& packet, const HEADER& header)
{
    return packet << static_cast<sf::Uint8>(header);
}

//DataPlayer

sf::Packet& operator>>(sf::Packet& packet, NetworkDataPlayer& data_player)
{
    return packet >> data_player.header >> data_player.name >> data_player.ip >> data_player.port >> data_player.dataGhost >> data_player.message;
}

sf::Packet& operator<<(sf::Packet& packet, const NetworkDataPlayer& data_player)
{
    return packet << data_player.header << data_player.name << data_player.ip << data_player.port << data_player.dataGhost << data_player.message;
}

std::ostream& operator<<(std::ostream& out, const QAngle angle)
{
    return out << "x : " << angle.x << "; y : " << angle.y << "; z : " << angle.z;
}

//PlayerInfo
struct PlayerInfo {
    sf::IpAddress ip;
    unsigned short int port;
    std::string name;
};

//Functions

/*
Wait until the socket receive an update of a client
Return the packet received
*/
bool ReceivePacket(sf::Packet& packet, sf::UdpSocket& socket, sf::IpAddress& ip_sender, unsigned short& port_sender, sf::SocketSelector& selector);


//Send the packet specified to the client
void SendPacket(sf::UdpSocket& socket, sf::Packet& packet, const sf::IpAddress& ip_client, const unsigned short& port);
//void SendPacket(const sf::TcpSocket& socket, const sf::Packet& packet);

//Listen for a specific port and manage connections and disconnections
void TCPcheck(bool& run, std::map<sf::IpAddress, NetworkDataPlayer>& player_pool);

//Listen for a client to connect and add the player to the player pool
void CheckNewConnection(sf::TcpListener& listener, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, NetworkDataPlayer>& player_pool, sf::SocketSelector& selector);

//Disconnect a player
void Disconnect(const NetworkDataPlayer& data, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, NetworkDataPlayer>& player_pool);

//Stop the server
void StopServer(bool& stopServer, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, NetworkDataPlayer>& player_pool, sf::Packet& packet);

std::mutex mutex;
std::mutex mutex2;
std::mutex mutex3;
std::condition_variable threadFinishedCondVar;
std::condition_variable mainFinishedCondVar;
std::atomic<bool> mainFinished = true;
std::atomic<bool> threadFinished = false;

/*
Manage multiple Portal 2 clients
Packets form :
	HEADER : Information about the packet received
	{
		NONE / CONNECT / UPDATE / DISCONNECT / STOP_SERVER / MESSAGE
	}

	NetworkDataPlayer : Network informations + ghost infos
	{
		HEADER header
		std::string    -> name
		std::string   -> ip adress
		unsigned short -> port
		DataGhost      -> ghost infos
		{
			QAngle         -> position
			QAngle         -> view angle
			char[64]       -> current map
		}
	}
*/
int main()
{

    //UDP
    std::map<sf::IpAddress, NetworkDataPlayer> player_pool;

    unsigned short int port = 53000;
    sf::UdpSocket UDPsocket;
    if (UDPsocket.bind(port) != sf::Socket::Done) {
        //error
        std::cout << "Cant bind socket to port " << port << std::endl;
        std::getchar();
        return 0;
    }

    sf::SocketSelector selector;
    selector.add(UDPsocket);

    std::cout << "Server started on <" << sf::IpAddress::getPublicAddress() << "> (Local : " << sf::IpAddress::getLocalAddress() << ") ; Port: " << UDPsocket.getLocalPort() << ">" << std::endl;

    //TCP

    bool stopServer = true;
    std::thread TCPthread(TCPcheck, std::ref(stopServer), std::ref(player_pool));
    TCPthread.detach();

    int size = 0;
    while (stopServer) {
        sf::IpAddress ip_sender;
        unsigned short port_sender;

        sf::Packet packet;
        bool receivedData;
        if (!ReceivePacket(packet, UDPsocket, ip_sender, port_sender, selector)) {
            receivedData = false;
        } else {
            receivedData = true;
        }

        std::unique_lock<std::mutex> lck(mutex);
        threadFinishedCondVar.wait(lck, [] { return threadFinished.load(); }); //Wait for the thread function to finish updating pools
        mainFinished = false;

        if (receivedData) {
            NetworkDataPlayer data;
            if (packet >> data) {
                if (data.header == HEADER::UPDATE) {
                    NetworkDataPlayer confirmation = data;
                    confirmation.message = "lol";
                    packet << confirmation;
                    for (auto& player_it : player_pool) {
                        SendPacket(UDPsocket, packet, player_it.second.ip, player_it.second.port); //Send update to other clients
                    }
                } else if (data.header == HEADER::PING) {
                    SendPacket(UDPsocket, packet, ip_sender, port_sender);
                    NetworkDataPlayer data;
                    packet >> data;
                    std::cout << "Ping from " + data.name + " !" << std::endl;
                }

                if (!data.message.empty()) {
                    std::cout << data.name << " says : \"" << data.message << "\"" << std::endl;
                }
            }
        }
        mainFinished = true;
        mainFinishedCondVar.notify_one();
    }

    std::getchar();

    return 0;
}

bool ReceivePacket(sf::Packet& packet, sf::UdpSocket& socket, sf::IpAddress& ip_sender, unsigned short& port_sender, sf::SocketSelector& selector)
{
    if (selector.wait(sf::seconds(1))) {
        if (socket.receive(packet, ip_sender, port_sender) != sf::Socket::Done) {
            //error
            std::cout << "Error : Can't receive packet" << std::endl;
            return false;
        }
    } else {
        return false;
    }
    return true;
}

void SendPacket(sf::UdpSocket& socket, sf::Packet& packet, const sf::IpAddress& ip_client, const unsigned short& port)
{
    if (socket.send(packet, ip_client, port) != sf::Socket::Done) {
        std::cout << "Error : Can't send packet" << std::endl;
    }
}

void TCPcheck(bool& stopServer, std::map<sf::IpAddress, NetworkDataPlayer>& player_pool)
{
    //TCP
    sf::SocketSelector selector;
    std::vector<std::shared_ptr<sf::TcpSocket>> socket_pool;
    sf::TcpListener listener;
    listener.setBlocking(false);

    if (listener.listen(53000) != sf::Socket::Done) {
        //Error
        stopServer = false;
    }

    while (stopServer) {

        CheckNewConnection(listener, socket_pool, player_pool, selector); //Check if someone wants to connect

        if (selector.wait(sf::seconds(5))) {
            for (auto& socket : socket_pool) {
                if (selector.isReady(*socket)) {
                    sf::Packet packet;
                    socket->receive(packet);
                    NetworkDataPlayer data;
                    packet >> data;
                    if (data.header == HEADER::DISCONNECT) {
                        Disconnect(data, socket_pool, player_pool);
                    } else if (data.header == HEADER::STOP_SERVER) {
                        StopServer(stopServer, socket_pool, player_pool, packet);
                    }
                }
            }
        }
    }
}

void CheckNewConnection(sf::TcpListener& listener, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, NetworkDataPlayer>& player_pool, sf::SocketSelector& selector)
{

    std::shared_ptr<sf::TcpSocket> client(new sf::TcpSocket);

    if (listener.accept(*client) != sf::Socket::Done) {
        return;
    }

    sf::Packet connection_packet;
    client->receive(connection_packet);

    NetworkDataPlayer connect_data;
    connection_packet >> connect_data;

    sf::IpAddress ip_sender = client->getRemoteAddress();
    selector.add(*client);

    std::unique_lock<std::mutex> lck(mutex3);
    mainFinishedCondVar.wait(lck, [] { return mainFinished.load(); }); //Wait for the main function to finish updating ghosts
    threadFinished = false;

    player_pool.insert({ ip_sender, connect_data });
    socket_pool.push_back(client);

    threadFinished = true;
    threadFinishedCondVar.notify_one();

    std::cout << "New player connected: Hello <" << client->getRemoteAddress() << "; "
              << client->getRemotePort() << "; "
              << connect_data.name << "> !" << std::endl;

    for (int i = 0; i < socket_pool.size(); ++i) {
        if (socket_pool[i]->getRemoteAddress() == ip_sender) { //Send confirmation

            sf::Packet confirm_packet;
            confirm_packet << static_cast<sf::Uint32>(socket_pool.size());
            for (auto& it : player_pool) {
                confirm_packet << it.second; //Send every player connected informations
            }
            std::string message = "You have been succesfully connected to the server !\n Players connected : "
                + std::to_string(socket_pool.size()) + "\n";
            confirm_packet << message;

            socket_pool[i]->send(confirm_packet);
        } else { //Update other players
            NetworkDataPlayer confirmation = connect_data;
            confirmation.message = "New player connected ! Say hello to :" + player_pool[ip_sender].name + " !\n";

            sf::Packet confirm_packet;
            confirm_packet << confirmation;
            socket_pool[i]->send(confirm_packet); //Send new player to other clients
        }
    }
}

void Disconnect(const NetworkDataPlayer& data, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, NetworkDataPlayer>& player_pool)
{
    std::cout << "Player " << data.name << " has disconnected !" << std::endl;
    int id = 0;
    for (int i = 0; i < socket_pool.size(); ++i) {
        //Update other players
        NetworkDataPlayer confirmation = data;
        confirmation.message = "Player " + data.name + " has disconnected !\n";

        sf::Packet confirm_packet;
        confirm_packet << confirmation;
        socket_pool[i]->send(confirm_packet); //Send disconnection to other clients
    }

    std::unique_lock<std::mutex> lck(mutex2);
    mainFinishedCondVar.wait(lck, [] { return mainFinished.load(); }); //Wait for the main function to finish updating ghosts
    threadFinished = false;

    player_pool.erase(data.ip);
    socket_pool[id]->disconnect();
    socket_pool.erase(socket_pool.begin() + id);

    threadFinished = true;
    threadFinishedCondVar.notify_one();
}

void StopServer(bool& stopServer, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, NetworkDataPlayer>& player_pool, sf::Packet& packet)
{
    std::cout << "STOP_SERVER received. Server will stop ! Press Enter to quit ..." << std::endl;
    stopServer = false;
    for (auto& it : socket_pool) {
        it->send(packet); //Send disconnection to other clients
    }
}