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
    DISCONNECT,
    STOP_SERVER,
    MAP_CHANGE,
    MESSAGE,
    UPDATE,
};

struct DataGhost {
    QAngle position;
    QAngle view_angle;
};

//DataGhost

sf::Packet& operator>>(sf::Packet& packet, QAngle& angle)
{
    return packet >> angle.x >> angle.y >> angle.z;
}

sf::Packet& operator>>(sf::Packet& packet, DataGhost& dataGhost)
{
    return packet >> dataGhost.position >> dataGhost.view_angle;
}

sf::Packet& operator<<(sf::Packet& packet, const QAngle& angle)
{
    return packet << angle.x << angle.y << angle.z;
}

sf::Packet& operator<<(sf::Packet& packet, const DataGhost& dataGhost)
{
    return packet << dataGhost.position << dataGhost.view_angle;
}

//HEADER

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

//PlayerInfo
struct PlayerInfo {
    sf::IpAddress ip;
    unsigned short int port;
    std::string name;
    DataGhost dataGhost;
    std::string currentMap;
    unsigned int socketID; //To handle crash
    //std::string modelName;
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
void TCPcheck(bool& run, std::vector<std::shared_ptr<sf::TcpSocket>>& socketpool, std::map<sf::IpAddress, PlayerInfo>& player_pool);

//Listen for a client to connect and add the player to the player pool
void CheckNewConnection(sf::TcpListener& listener, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, PlayerInfo>& player_pool, sf::SocketSelector& selector);

//Disconnect a player
void Disconnect(const sf::Uint32& ID, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, PlayerInfo>& player_pool, bool hasCrashed = false);

//Stop the server
void StopServer(bool& stopServer, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool);

//Signal map change
void ChangeMap(const sf::Uint32& ID, const std::string& map, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, PlayerInfo>& player_pool);

//Retransmit message to every players
void SendMessage(const sf::Uint32& ID, const std::string& message, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool);

std::mutex mutex;
std::mutex mutex2;
std::mutex mutex3;
std::condition_variable threadFinishedCondVar;
std::condition_variable mainFinishedCondVar;
std::atomic<bool> mainFinished = true;
std::atomic<bool> threadFinished = false;

/*
Manage multiple Portal 2 clients
Packets contains :
	HEADER : Information about the packet received
	{
		NONE / PING / CONNECT / UPDATE / DISCONNECT / STOP_SERVER / MAP_CHANGE / MESSAGE
	}

	DataGhost      -> ghost infos
	{
		QAngle         -> position
		QAngle         -> view angle
	}

	PlayerInfo
	{
		sf::IpAddress		-> ip address
		unsigned short int  -> port
		std::string name	-> player name
	}

	 if HEADER == CONNECT :
	 Server receive : TCPpacket << HEADER << sf::Uint16 UDP port << std::string name << DataGhost data << std::string currentMap << std::string modelName
	 Server send confirmation : TCPpacket << sf::Uint32 nb_player << sf::Uint32 id << std::string name << DataGhost data  << std::string currentMap << std::string modelName : id, name, data, currentMap, and modelName of every player connected
	 Server send to other : TCPpacket << HEADER << sf::Uint32 ID << std::string name << DataGhost data << std::string currentMap << std::string modelName
	 sf::Uint32 ID is just the ip address converted to sf::Uint32

	 if HEADER == DISCONNECT :
	 Server receive :TCPpacket << HEADER
	 Server send : TCPpacket << HEADER << sf::Uint32 ID

	 if HEADER == STOP_SERVER :
	 Server receive and send : TCPpacket << HEADER

	 if HEADER == CHANGE_MAP :
	 Server receive : TCPpacket << HEADER << std::string currentMap
	 Server send : TCPpacket << HEADER << sf::Uint32 ID << std::string currentMap

	 if HEADER == MESSAGE :
	 Server receive : TCPpacket << HEADER << std::string message
	 Server send : TCPpacket << HEADER << sf::Uint32 ID << std::string message

	 if HEADER == UPDATE :
	 Server receive : UDPpacket << HEADER << DataGhost
	 Server send : UDPpacket << HEADER << sf::Uint32 ID << DataGhost

	 if HEADER == PING :
	 Server receive and send : UDPpacket << HEADER
*/
int main()
{

    //UDP
    std::map<sf::IpAddress, PlayerInfo> player_pool;

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
    std::vector<std::shared_ptr<sf::TcpSocket>> socket_pool;
    bool stopServer = true;
    std::thread TCPthread(TCPcheck, std::ref(stopServer), std::ref(socket_pool), std::ref(player_pool));
    TCPthread.detach();

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
            HEADER header;
            if (packet >> header) {
                if (header == HEADER::UPDATE) {
                    DataGhost data;
                    packet >> data;
                    sf::Packet update_packet;
                    update_packet << header << ip_sender.toInteger() << data;
                    for (auto& player_it : player_pool) {
                        if (player_it.first != ip_sender) {
                            SendPacket(UDPsocket, update_packet, player_it.first, player_it.second.port); //Send update to other clients
                        }
                    }
                } else if (header == HEADER::PING) {
                    SendPacket(UDPsocket, packet, ip_sender, port_sender);
                    std::cout << "Ping from " + player_pool[ip_sender].name + " !" << std::endl;
                }
            }
        }
        mainFinished = true;
        mainFinishedCondVar.notify_one();
    }

    for (auto& it : socket_pool) {
        it->disconnect();
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

void TCPcheck(bool& stopServer, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, PlayerInfo>& player_pool)
{
    //TCP
    sf::SocketSelector selector;
    sf::TcpListener listener;
    listener.setBlocking(false);

    if (listener.listen(53000) != sf::Socket::Done) {
        //Error
        stopServer = false;
    }

    while (stopServer) {

        CheckNewConnection(listener, socket_pool, player_pool, selector); //Check if someone wants to connect

        unsigned int crashed = -1;
        if (selector.wait(sf::seconds(5))) {
            for (size_t id = 0; id < socket_pool.size(); ++id) {
                if (socket_pool[id]->getRemoteAddress() != sf::IpAddress::None) {
                    if (selector.isReady(*socket_pool[id])) {
                        sf::Packet packet;
                        socket_pool[id]->receive(packet);
                        HEADER header;
                        packet >> header;
                        if (header == HEADER::DISCONNECT) {
                            sf::Uint32 ID = socket_pool[id]->getRemoteAddress().toInteger();
                            Disconnect(ID, socket_pool, player_pool);
                        } else if (header == HEADER::STOP_SERVER) {
                            std::cout << "STOP_SERVER received. Server will stop ! Press Enter to quit ..." << std::endl;
                            StopServer(stopServer, socket_pool);
                        } else if (header == HEADER::MAP_CHANGE) {
                            std::string newMap;
                            packet >> newMap;
                            ChangeMap(socket_pool[id]->getRemoteAddress().toInteger(), newMap, socket_pool, player_pool);
                        } else if (header == HEADER::MESSAGE) {
                            std::string message;
                            packet >> message;
                            std::cout << player_pool[socket_pool[id]->getRemoteAddress()].name << " : " << message << std::endl;

                            sf::Uint32 ID = socket_pool[id]->getRemoteAddress().toInteger();
                            SendMessage(ID, message, socket_pool);
                        }
                    }
                } else {
                    crashed = id;
                }
            }
        }

        if (crashed != -1) {
            Disconnect(crashed, socket_pool, player_pool, true);
        }
    }
}

void CheckNewConnection(sf::TcpListener& listener, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, PlayerInfo>& player_pool, sf::SocketSelector& selector)
{

    std::shared_ptr<sf::TcpSocket> client(new sf::TcpSocket);

    if (listener.accept(*client) != sf::Socket::Done) {
        return;
    }

    sf::IpAddress ip_sender = client->getRemoteAddress();
    if (player_pool.find(ip_sender) != player_pool.end()) { //Check if player is already connected
        return;
    }

    selector.add(*client);
    sf::Packet connection_packet;
    client->receive(connection_packet);

    std::unique_lock<std::mutex> lck(mutex3);
    mainFinishedCondVar.wait(lck, [] { return mainFinished.load(); }); //Wait for the main function to finish updating ghosts
    threadFinished = false;

    HEADER header;
    sf::Uint16 port_sender;
    std::string name;
    DataGhost data;
    std::string currentMap;
    std::string modelName;
    connection_packet >> header >> port_sender >> name >> data >> currentMap; // >> modelName;

    player_pool.insert({ ip_sender, { ip_sender, port_sender, name, data, currentMap, socket_pool.size()}}); //, modelName } });
    socket_pool.push_back(client);

    threadFinished = true;
    threadFinishedCondVar.notify_one();

    std::cout << "New player connected: Hello <" << ip_sender << "; "
              << port_sender << "; "
              << name << "> !" << std::endl;

    for (auto& socket : socket_pool) {
        if (socket->getRemoteAddress() == ip_sender) { //Send confirmation
            sf::Packet confirm_packet;
            confirm_packet << static_cast<sf::Uint32>(socket_pool.size() - 1);
            for (auto& player : player_pool) {
                if (player.first != ip_sender) {
                    confirm_packet << player.first.toInteger() << player.second.name << player.second.dataGhost << player.second.currentMap; // << player.second.modelName; //Send every player connected informations
                }
            }
            socket->send(confirm_packet);

        } else { //Update other players
            sf::Packet confirm_packet;
            confirm_packet << header << ip_sender.toInteger() << name << data << currentMap; // << modelName;
            socket->send(confirm_packet); //Send new player to other clients
        }
    }
}

void Disconnect(const sf::Uint32& ID, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, PlayerInfo>& player_pool, bool hasCrashed)
{
    if (!hasCrashed) {
        sf::IpAddress ip_sender = sf::IpAddress(ID);
        {
            std::unique_lock<std::mutex> lck(mutex2);
            std::cout << "Player " << player_pool[ip_sender].name << " has disconnected !" << std::endl;
        }
        int id = 0;
        for (int i = 0; i < socket_pool.size(); ++i) {
            //Update other players
            sf::Packet confirm_packet;
            confirm_packet << HEADER::DISCONNECT << ID;
            socket_pool[i]->send(confirm_packet); //Send disconnection to other clients
            if (socket_pool[i]->getRemoteAddress() == ip_sender) {
                id = i;
            }
        }

        std::unique_lock<std::mutex> lck(mutex2);
        mainFinishedCondVar.wait(lck, [] { return mainFinished.load(); }); //Wait for the main function to finish updating ghosts
        threadFinished = false;

        player_pool.erase(ip_sender);
        socket_pool[id]->disconnect();
        socket_pool.erase(socket_pool.begin() + id);

        threadFinished = true;
        threadFinishedCondVar.notify_one();
    } else {
        std::unique_lock<std::mutex> lck(mutex2);
        mainFinishedCondVar.wait(lck, [] { return mainFinished.load(); }); //Wait for the main function to finish updating ghosts
        threadFinished = false;

        socket_pool.erase(socket_pool.begin() + ID);
        auto playerID = player_pool.begin();
        for (auto player = player_pool.begin(); player != player_pool.end(); ++player) {
            if (player->second.socketID == ID) {
                playerID = player;
                player = player_pool.end();
            }
        }
        std::cout << "Player " << playerID->second.name << " has disconnected !" << std::endl;
        int id = 0;
        sf::Packet confirm_packet;
        confirm_packet << HEADER::DISCONNECT << playerID->first.toInteger();
        for (auto& socket : socket_pool) {
            //Update other players
            socket->send(confirm_packet); //Send disconnection to other clients
        }

        player_pool.erase(playerID);

        threadFinished = true;
        threadFinishedCondVar.notify_one();
    }
}

void StopServer(bool& stopServer, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool)
{
    stopServer = false;
    sf::Packet packet;
    packet << HEADER::STOP_SERVER;
    for (auto& it : socket_pool) {
        it->send(packet); //Send disconnection to other clients
    }
}

void ChangeMap(const sf::Uint32& ID, const std::string& map, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool, std::map<sf::IpAddress, PlayerInfo>& player_pool)
{
    sf::Packet packet;
    packet << HEADER::MAP_CHANGE << ID << map;
    for (auto& socket : socket_pool) {
        if (socket->getRemoteAddress().toInteger() != ID) {
        socket->send(packet);
		}
    }
}

void SendMessage(const sf::Uint32& ID, const std::string& message, std::vector<std::shared_ptr<sf::TcpSocket>>& socket_pool)
{
    sf::Packet packet;
    packet << HEADER::MESSAGE << ID << message;
    for (auto& it : socket_pool) {
        it->send(packet); //Send disconnection to other clients
    }
}