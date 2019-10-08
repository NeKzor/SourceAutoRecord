#pragma once

#include "SFML/Network.hpp"
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
    COUNTDOWN,
    UPDATE,
};

struct DataGhost {
    QAngle position;
    QAngle view_angle;
};

//PlayerInfo
struct PlayerInfo {
    sf::IpAddress ip;
    unsigned short int port;
    std::string name;
    DataGhost dataGhost;
    std::string currentMap;
    unsigned int socketID; //To handle crash
    std::string modelName;
};

class NetworkManager {

private:
    std::thread TCPthread;
    std::thread UDPthread;
    unsigned short int port;
    std::map<sf::IpAddress, PlayerInfo> player_pool;
    std::vector<std::shared_ptr<sf::TcpSocket>> socket_pool;
    sf::UdpSocket UDPSocket;
    bool isConnected;
    bool runServer;

public:

public:
    NetworkManager(unsigned short int port = 53000);

	bool IsConnected();
    sf::IpAddress GetLocalIP();
    sf::IpAddress GetPublicIP();
    unsigned short int GetPort();


    bool ReceivePacket(sf::Packet& packet, sf::UdpSocket& socket, sf::IpAddress& ip_sender, unsigned short& port_sender, sf::SocketSelector& selector);
    void SendPacket(sf::UdpSocket& socket, sf::Packet& packet, const sf::IpAddress& ip_client, const unsigned short& port);

    //void SendPacket(const sf::TcpSocket& socket, const sf::Packet& packet);
    void TCPListening();
    void UDPListening();
    
	void CheckNewConnection(sf::TcpListener& listener, sf::SocketSelector& selector);
    void Disconnect(const sf::Uint32& ID, sf::SocketSelector& selector, bool hasCrashed = false);
    void StopServer(bool& stopServer);
    void ChangeMap(const sf::Uint32& ID, const std::string& map);
    void SendMessage(const sf::Uint32& ID, const std::string& message);
	
};