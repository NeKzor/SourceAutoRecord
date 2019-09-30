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

class NetworkGhostPlayer : public Feature {

private:
    std::vector<NetworkDataPlayer> networkGhosts;
    bool isConnected;
    sf::SocketSelector selector;
    std::condition_variable waitForPaused;
    //std::mutex mutex;

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
    GhostEntity* SetupGhost(NetworkDataPlayer&);
    void UpdatePlayer();

public:
    NetworkGhostPlayer();

    void ConnectToServer(std::string, unsigned short port);
    void Disconnect(bool forced);
    void StopServer();
    bool IsConnected();

    void SendNetworkData(NetworkDataPlayer&);
    bool ReceiveNetworkData(sf::Packet& packet, int timeout);

    void StartThinking();
    void PauseThinking();

    NetworkDataPlayer CreateNetworkData();
    DataGhost GetPlayerData();
    GhostEntity* GetGhostByID(std::string& ID);
    void SetPosAng(std::string ID, Vector position, Vector angle);
    void UpdateCurrentMap();
};

extern NetworkGhostPlayer* networkGhostPlayer;

extern Command sar_ghost_connect_to_server;
extern Command sar_ghost_disconnect;
extern Command sar_ghost_stop_server;
extern Command sar_ghost_name;
