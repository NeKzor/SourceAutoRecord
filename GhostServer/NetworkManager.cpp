#include "NetworkManager.h"


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

	 if HEADER == PING :
		Server receive and send : TCPpacket << HEADER

	 if HEADER == COUNTDOWN :
		Server receive and send : TCPpacket << HEADER

	 if HEADER == UPDATE :
		Server receive : UDPpacket << HEADER << DataGhost
		Server send : UDPpacket << HEADER << sf::Uint32 ID << DataGhost
*/


std::mutex mutex;
std::mutex mutex2;
std::mutex mutex3;
std::condition_variable TCPfinishedCondVar;
std::condition_variable UDPfinishedCondVar;
std::atomic<bool> UDPfinished = true;
std::atomic<bool> TCPfinished = false;

NetworkManager::NetworkManager(unsigned short int port)
    : isConnected(false)
{
	//UDP
    this->port = port;
    if (this->UDPSocket.bind(port) != sf::Socket::Done) {
        //error
        std::cout << "Cant bind socket to port " << port << std::endl;
        std::getchar();
		//TODO : Handle errors
    }

    this->runServer = true;
    UDPthread = std::thread(&NetworkManager::UDPListening, this);
    UDPthread.detach();

	//TCP
    TCPthread = std::thread(&NetworkManager::TCPListening, this);
    TCPthread.detach();

    this->isConnected = true;
}

bool NetworkManager::IsConnected()
{
    return this->isConnected;
}

sf::IpAddress NetworkManager::GetLocalIP()
{
    return sf::IpAddress::getLocalAddress();
}

sf::IpAddress NetworkManager::GetPublicIP()
{
    return sf::IpAddress::getPublicAddress();
}


unsigned short int NetworkManager::GetPort()
{
    return this->port;
}


/*
Wait until the socket receive an update of a client
Return the packet received
*/
bool NetworkManager::ReceivePacket(sf::Packet& packet, sf::UdpSocket& socket, sf::IpAddress& ip_sender, unsigned short& port_sender, sf::SocketSelector& selector)
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

//Send the packet specified to the client
void NetworkManager::SendPacket(sf::UdpSocket& socket, sf::Packet& packet, const sf::IpAddress& ip_client, const unsigned short& port)
{
    if (socket.send(packet, ip_client, port) != sf::Socket::Done) {
        std::cout << "Error : Can't send packet" << std::endl;
    }
}

void NetworkManager::UDPListening()
{
    sf::SocketSelector selector;
    selector.add(this->UDPSocket);

    while (this->runServer) {
        sf::IpAddress ip_sender;
        unsigned short port_sender;

        sf::Packet packet;
        bool receivedData;
        if (!this->ReceivePacket(packet, this->UDPSocket, ip_sender, port_sender, selector)) {
            receivedData = false;
        } else {
            receivedData = true;
        }

        {
            std::unique_lock<std::mutex> lck(mutex);
            TCPfinishedCondVar.wait(lck, [] { return TCPfinished.load(); }); //Wait for the TCPListener function to finish updating pools
            UDPfinished = false;
        }

        if (receivedData) {
            HEADER header;
            if (packet >> header) {
                if (header == HEADER::UPDATE) {
                    DataGhost data;
                    packet >> data;
                    sf::Packet update_packet;
                    update_packet << header << ip_sender.toInteger() << data;
                    for (auto& player_it : this->player_pool) {
                        if (player_it.first != ip_sender) {
                            SendPacket(this->UDPSocket, update_packet, player_it.first, player_it.second.port); //Send update to other clients
                        }
                    }
                }
            }
        }
        UDPfinished = true;
        UDPfinishedCondVar.notify_one();
    }

    for (auto& it : this->socket_pool) {
        it->disconnect();
    }
}

//Listen for a specific port and manage connections and disconnections
void NetworkManager::TCPListening()
{
    //TCP
    sf::SocketSelector selector;
    sf::TcpListener listener;
    listener.setBlocking(true);

    if (listener.listen(this->port) != sf::Socket::Done) {
        //Error
        this->runServer = false;
    }

    while (this->runServer) {

        CheckNewConnection(listener, selector); //Check if someone wants to connect

        unsigned int crashed = -1;
        if (selector.wait(sf::seconds(5))) {
            for (size_t id = 0; id < this->socket_pool.size(); ++id) {
                if (this->socket_pool[id]->getRemoteAddress() != sf::IpAddress::None) {
                    if (selector.isReady(*this->socket_pool[id])) {
                        sf::Packet packet;
                        this->socket_pool[id]->receive(packet);
                        HEADER header;
                        packet >> header;
                        if (header == HEADER::DISCONNECT) {
                            sf::Uint32 ID = this->socket_pool[id]->getRemoteAddress().toInteger();
                            Disconnect(ID, selector);
                        } else if (header == HEADER::STOP_SERVER) {
                            std::cout << "STOP_SERVER received. Server will stop ! Press Enter to quit ..." << std::endl;
                            StopServer(this->runServer);
                        } else if (header == HEADER::MAP_CHANGE) {
                            std::string newMap;
                            packet >> newMap;
                            ChangeMap(this->socket_pool[id]->getRemoteAddress().toInteger(), newMap);
                        } else if (header == HEADER::MESSAGE) {
                            std::string message;
                            packet >> message;
                            std::cout << this->player_pool[this->socket_pool[id]->getRemoteAddress()].name << " : " << message << std::endl;

                            sf::Uint32 ID = this->socket_pool[id]->getRemoteAddress().toInteger();
                            SendMessage(ID, message);
                        } else if (header == HEADER::PING) {
                            sf::Packet packet_ping;
                            packet_ping << HEADER::PING;
                            this->socket_pool[id]->send(packet_ping);
                            std::cout << "Ping from " + this->player_pool[this->socket_pool[id]->getRemoteAddress()].name + " !" << std::endl;
                        } else if (header == HEADER::COUNTDOWN) {
                            sf::Packet countdown_packet;
                            countdown_packet << HEADER::COUNTDOWN;
                            for (auto& socket : this->socket_pool) {
                                socket->send(packet);
                            }
                        }
                    }
                } else {
                    crashed = id;
                }
            }
        } else if(this->socket_pool.size() == 0) {
            listener.setBlocking(true);
		}

        if (crashed != -1) {
            Disconnect(crashed, selector, true);
        }
    }
}

//Listen for a client to connect and add the player to the player pool
void NetworkManager::CheckNewConnection(sf::TcpListener& listener, sf::SocketSelector& selector)
{
    std::shared_ptr<sf::TcpSocket> client(new sf::TcpSocket);

    if (listener.accept(*client) != sf::Socket::Done) {
        return;
    }
    if (this->socket_pool.size() == 0) {
        listener.setBlocking(false);
	}

    sf::IpAddress ip_sender = client->getRemoteAddress();
    if (this->player_pool.find(ip_sender) != this->player_pool.end()) { //Check if player is already connected
        return;
    }

    selector.add(*client);
    sf::Packet connection_packet;
    client->receive(connection_packet);

    std::unique_lock<std::mutex> lck(mutex3);
    UDPfinishedCondVar.wait(lck, [] { return UDPfinished.load(); }); //Wait for the main function to finish updating ghosts
    TCPfinished = false;

    HEADER header;
    sf::Uint16 port_sender;
    std::string name;
    DataGhost data;
    std::string currentMap;
    std::string modelName;
    connection_packet >> header >> port_sender >> name >> data >> currentMap >> modelName;

    this->player_pool.insert({ ip_sender, { ip_sender, port_sender, name, data, currentMap, this->socket_pool.size(), modelName } });
    this->socket_pool.push_back(client);

    TCPfinished = true;
    TCPfinishedCondVar.notify_one();

    std::cout << "New player connected: Hello <" << ip_sender << "; "
              << port_sender << "; "
              << name << "> !" << std::endl;

    for (auto& socket : this->socket_pool) {
        if (socket->getRemoteAddress() == ip_sender) { //Send confirmation
            sf::Packet confirm_packet;
            confirm_packet << static_cast<sf::Uint32>(this->socket_pool.size() - 1);
            for (auto& player : this->player_pool) {
                if (player.first != ip_sender) {
                    confirm_packet << player.first.toInteger() << player.second.name << player.second.dataGhost << player.second.currentMap << player.second.modelName; //Send every player connected informations
                }
            }
            socket->send(confirm_packet);

        } else { //Update other players
            sf::Packet confirm_packet;
            confirm_packet << header << ip_sender.toInteger() << name << data << currentMap << modelName;
            socket->send(confirm_packet); //Send new player to other clients
        }
    }
}

//Disconnect a player
void NetworkManager::Disconnect(const sf::Uint32& ID, sf::SocketSelector& selector, bool hasCrashed)
{
    sf::IpAddress ip_sender = sf::IpAddress(ID);
    if (!hasCrashed) {
        {
            std::unique_lock<std::mutex> lck(mutex2);
            std::cout << "Player " << this->player_pool[ip_sender].name << " has disconnected !" << std::endl;
        }
        int id = 0;
        for (int i = 0; i < this->socket_pool.size(); ++i) {
            //Update other players
            sf::Packet confirm_packet;
            confirm_packet << HEADER::DISCONNECT << ID;
            this->socket_pool[i]->send(confirm_packet); //Send disconnection to other clients
            if (this->socket_pool[i]->getRemoteAddress() == ip_sender) {
                id = i;
            }
        }

        std::unique_lock<std::mutex> lck(mutex2);
        UDPfinishedCondVar.wait(lck, [] { return UDPfinished.load(); }); //Wait for the main function to finish updating ghosts
        TCPfinished = false;

        this->player_pool.erase(ip_sender);
        selector.remove(*this->socket_pool[id]);
        this->socket_pool[id]->disconnect();
        this->socket_pool.erase(this->socket_pool.begin() + id);

        TCPfinished = true;
        TCPfinishedCondVar.notify_one();
    } else {
        std::unique_lock<std::mutex> lck(mutex2);
        UDPfinishedCondVar.wait(lck, [] { return UDPfinished.load(); }); //Wait for the main function to finish updating ghosts
        TCPfinished = false;

        int id = 0;
        for (int i = 0; i < this->socket_pool.size(); ++i) {
            if (this->socket_pool[i]->getRemoteAddress() == ip_sender) {
                id = i;
            }
        }
        selector.remove(*this->socket_pool[id]);
        this->socket_pool.erase(this->socket_pool.begin() + ID);
        auto playerID = this->player_pool.begin();
        for (auto player = this->player_pool.begin(); player != this->player_pool.end(); ++player) {
            if (player->second.socketID == ID) {
                playerID = player;
                player = this->player_pool.end();
            }
        }
        std::cout << "Player " << playerID->second.name << " has disconnected !" << std::endl;
        sf::Packet confirm_packet;
        confirm_packet << HEADER::DISCONNECT << playerID->first.toInteger();
        for (auto& socket : this->socket_pool) {
            //Update other players
            socket->send(confirm_packet); //Send disconnection to other clients
        }

        this->player_pool.erase(playerID);

        TCPfinished = true;
        TCPfinishedCondVar.notify_one();
    }
}

//Stop the server
void NetworkManager::StopServer(bool& stopServer)
{
    sf::Packet packet;
    packet << HEADER::STOP_SERVER;
    for (auto& it : this->socket_pool) {
        it->send(packet); //Send disconnection to other clients
    }
    stopServer = false;
}

//Signal map change
void NetworkManager::ChangeMap(const sf::Uint32& ID, const std::string& map)
{
    sf::Packet packet;
    packet << HEADER::MAP_CHANGE << ID << map;
    for (auto& socket : this->socket_pool) {
        if (socket->getRemoteAddress().toInteger() != ID) {
            socket->send(packet);
        }
    }
}

//Retransmit message to every players
void NetworkManager::SendMessage(const sf::Uint32& ID, const std::string& message)
{
    sf::Packet packet;
    packet << HEADER::MESSAGE << ID << message;
    for (auto& it : this->socket_pool) {
        it->send(packet); //Send disconnection to other clients
    }
}