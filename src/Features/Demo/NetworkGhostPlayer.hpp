#pragma once
#include "Features/Feature.hpp"
#include "GhostEntity.hpp"

#include "Command.hpp"
#include "Features/Demo/Demo.hpp"
#include "Utils/SDK.hpp"
#include "Variable.hpp"

#include <SFML/Network.hpp>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>

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

class NetworkGhostPlayer : public Feature {

private:
    bool isConnected;
    sf::SocketSelector selector;
    std::condition_variable waitForPaused;

    public : sf::IpAddress ip_client;
    std::vector<GhostEntity*> ghostPool;
    std::string name;
    sf::IpAddress ip_server;
    unsigned short port_server;
    sf::UdpSocket socket;
    sf::TcpSocket tcpSocket;
    std::thread networkThread;
    std::thread TCPThread;
    std::atomic<bool> runThread;
    std::atomic<bool> pauseThread;
    std::chrono::steady_clock clock;
    std::chrono::time_point<std::chrono::steady_clock> start;

private:
    void NetworkThink();
    void CheckConnection();
    GhostEntity* SetupGhost(sf::Uint32& ID, std::string name, DataGhost&, std::string&);
    void UpdatePlayer();

public:
    NetworkGhostPlayer();

    void ConnectToServer(std::string, unsigned short port);
    void Disconnect(bool forced);
    void StopServer();
    bool IsConnected();

    int ReceivePacket(sf::Packet& packet, int timeout);

    void StartThinking();
    void PauseThinking();

    DataGhost GetPlayerData();
    GhostEntity* GetGhostByID(const sf::Uint32& ID);
    void SetPosAng(sf::Uint32& ID, Vector position, Vector angle);
    void UpdateCurrentMap();
    void UpdateGhostsCurrentMap();
};

extern NetworkGhostPlayer* networkGhostPlayer;

extern Command sar_ghost_connect_to_server;
extern Command sar_ghost_disconnect;
extern Command sar_ghost_stop_server;
extern Command sar_ghost_name;
