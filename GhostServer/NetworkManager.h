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

enum class HEADER {
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

struct EventInfo {
    HEADER header;
    std::string name;
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

//HEADER
sf::Packet& operator>>(sf::Packet& packet, HEADER& header);

sf::Packet& operator<<(sf::Packet& packet, const HEADER& header);

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
    bool stopped;
    sf::SocketSelector TCPselector;
    sf::TcpListener listener;
    std::vector<sf::Packet> eventList;

public:
public:
    NetworkManager(unsigned short int port = 53000);

    bool IsConnected();
    bool IsStopped();
    sf::IpAddress GetLocalIP();
    sf::IpAddress GetPublicIP();
    unsigned short int GetPort();

    bool ReceivePacket(sf::Packet& packet, sf::UdpSocket& socket, sf::IpAddress& ip_sender, unsigned short& port_sender, sf::SocketSelector& selector);

    void TCPListening();
    void UDPListening();

    void CheckNewConnection();
    std::string Disconnect(const sf::Uint32& ID, bool hasCrashed = false);
    void StopServer();
    void ChangeMap(const sf::Uint32& ID, const std::string& map);
    void SendMessage(const sf::Uint32& ID, const std::string& message);
    void StartCountdown(sf::Uint32 time);

    void GetEvent(std::vector<sf::Packet>& e);
};
