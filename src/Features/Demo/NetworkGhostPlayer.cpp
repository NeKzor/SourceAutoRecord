#include "NetworkGhostPlayer.hpp"

#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Utils/SDK.hpp"
#include <chrono>

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

//DataPlayer

sf::Packet& operator>>(sf::Packet& packet, NetworkDataPlayer& data_player)
{
    return packet >> data_player.header >> data_player.name >> data_player.ip >> data_player.port >> data_player.dataGhost >> data_player.message;
}

sf::Packet& operator<<(sf::Packet& packet, const NetworkDataPlayer& data_player)
{
    return packet << data_player.header << data_player.name << data_player.ip << data_player.port << data_player.dataGhost << data_player.message;
}

NetworkGhostPlayer* networkGhostPlayer;

std::mutex mutex;

NetworkGhostPlayer::NetworkGhostPlayer()
    : ip_server("localhost")
    , port_server(53000)
    , name("FrenchSaves10Ticks")
    , networkGhosts()
    , runThread(false)
    , pauseThread(true)
    , isConnected(false)
    , networkThread()
    , TCPThread()
    , ghostPool()
    , tcpSocket()
{
    this->hasLoaded = true;
}

void NetworkGhostPlayer::ConnectToServer(std::string ip, unsigned short port)
{
    if (tcpSocket.connect(ip, port, sf::seconds(5)) != sf::Socket::Done) {
        console->Warning("Timeout reached ! Can't connect to the server %s:%d !\n", ip.c_str(), port);
        return;
    }

    this->socket.bind(sf::Socket::AnyPort);
    this->selector.add(this->socket);

    this->ip_server = ip;
    this->port_server = port;
    NetworkDataPlayer data = this->CreateNetworkData();
    data.header = HEADER::CONNECT;

    sf::Packet connection_packet;
    connection_packet << data;
    tcpSocket.send(connection_packet);

    sf::SocketSelector tcpselector;
    tcpselector.add(tcpSocket);

    sf::Packet confirmation_packet;
    if (tcpselector.wait(sf::seconds(30))) {
        if (tcpSocket.receive(confirmation_packet) != sf::Socket::Done) {
            console->Warning("Error\n");
            return;
        }
    } else {
        console->Warning("Timeout reached ! Can't connect to the server %s:%d !\n", ip.c_str(), port);
        return;
    }

    sf::Uint32 nbPlayer;
    confirmation_packet >> nbPlayer;
    for (sf::Uint32 i = 0; i < nbPlayer; ++i) {
        NetworkDataPlayer tmp_player;
        confirmation_packet >> tmp_player;
        this->ghostPool.push_back(this->SetupGhost(tmp_player));
    }

    std::string message;
    confirmation_packet >> message;
    if (!message.empty()) {
        if (message == "Error: Timeout reached ! Can't connect to the server !\n" || message == "Error: Connexion to the server lost !\n") {
            console->Warning(message.c_str());
            return;
        } else {
            console->Print(message.c_str());
        }

        this->isConnected = true;
        this->StartThinking();
    }
}

void NetworkGhostPlayer::Disconnect(bool forced)
{
    this->runThread = false;
    this->waitForPaused.notify_one(); //runThread being false will make the thread stopping no matter if pauseThread is true or false

    NetworkDataPlayer data = this->CreateNetworkData();
    data.header = HEADER::DISCONNECT;
    sf::Packet packet;
    packet << data;
    this->tcpSocket.send(packet);

    for (auto& it : this->ghostPool) {
        it->Stop();
    }
    this->ghostPool.clear();
    this->isConnected = false;
    this->selector.clear();
    this->socket.unbind();
    this->tcpSocket.disconnect();
    while (this->networkThread.joinable()); //Check if the thread is dead
}

void NetworkGhostPlayer::StopServer()
{
    this->runThread = false;
    this->waitForPaused.notify_one(); //runThread being false will make the thread stopping no matter if pauseThread is true or false

    NetworkDataPlayer data = this->CreateNetworkData();
    data.header = HEADER::STOP_SERVER;
    sf::Packet packet;
    packet << data;
    this->tcpSocket.send(packet);

    NetworkDataPlayer confirmation;
    this->tcpSocket.setBlocking(true);
    while (confirmation.ip != this->ip_client) {
        sf::Packet confirmation_packet;
        this->tcpSocket.receive(confirmation_packet);
        confirmation_packet >> confirmation;
    }
    console->Print(confirmation.message.c_str());

    for (auto& it : this->ghostPool) {
        it->Stop();
    }
    this->ghostPool.clear();
    this->isConnected = false;
    this->selector.clear();
    this->socket.unbind();
    this->tcpSocket.disconnect();
}

bool NetworkGhostPlayer::IsConnected()
{
    return this->isConnected;
}

void NetworkGhostPlayer::SendNetworkData(NetworkDataPlayer& data)
{
    sf::Packet packet;
    packet << data;

    this->socket.send(packet, this->ip_server, this->port_server);
}

bool NetworkGhostPlayer::ReceiveNetworkData(sf::Packet& packet, int timeout)
{
    sf::IpAddress ip;
    unsigned short port;

    if (selector.wait(sf::seconds(timeout))) {
        this->socket.receive(packet, ip, port);
    } else {
        NetworkDataPlayer data;
        data.header = HEADER::NONE;
        if (this->IsConnected()) {
            data.message = "Error: Connexion to the server lost !\n";
            this->Disconnect(true);
        } else {
            data.message = "Error: Timeout reached ! Can't connect to the server !\n";
        }

        return false;
    }

    return true;
}

NetworkDataPlayer NetworkGhostPlayer::CreateNetworkData()
{
    NetworkDataPlayer networkData{ HEADER::NONE, this->name, this->ip_client.toString(), this->socket.getLocalPort() };
    DataGhost dataGhost{ { 0, 0, 0 }, { 0, 0, 0 }, engine->m_szLevelName };
    networkData.dataGhost = dataGhost;
    networkData.message = "";

    return networkData;
}

DataGhost NetworkGhostPlayer::GetPlayerData()
{
    DataGhost data = {
        VectorToQAngle(server->GetAbsOrigin(server->GetPlayer(GET_SLOT() + 1))),
        server->GetAbsAngles(server->GetPlayer(GET_SLOT() + 1))
    };
    data.currentMap = engine->m_szLevelName;

    return data;
}

GhostEntity* NetworkGhostPlayer::GetGhostByID(std::string& ID)
{
    for (auto it : this->ghostPool) {
        if (it->ID == ID) {
            return it;
        }
    }
    return nullptr;
}

void NetworkGhostPlayer::SetPosAng(std::string ID, Vector position, Vector angle)
{
    this->GetGhostByID(ID)->SetPosAng(position, angle);
}

void NetworkGhostPlayer::UpdateCurrentMap()
{
    this->ghostPool[0]->currentMap = engine->m_szLevelName;
}

void NetworkGhostPlayer::StartThinking()
{
    if (this->runThread) { //Already running (level change, load save)
        this->pauseThread = false;
        this->waitForPaused.notify_one();
    } else { //First time we connect
        this->runThread = true;
        this->pauseThread = false;
        this->waitForPaused.notify_one();
        this->networkThread = std::thread(&NetworkGhostPlayer::NetworkThink, this);
        this->networkThread.detach();
        this->TCPThread = std::thread(&NetworkGhostPlayer::CheckConnection, this);
        this->TCPThread.detach();
    }
}

void NetworkGhostPlayer::PauseThinking()
{
    this->pauseThread = true;
}

//Called on another thread
void NetworkGhostPlayer::NetworkThink()
{
    for (auto& it : this->ghostPool) {
        if (it->currentMap != engine->m_szLevelName) { //If on a different map
            it->sameMap = false;
        } else if (it->currentMap == engine->m_szLevelName && !it->sameMap) { //If previously on a different map but now on the same one
            it->sameMap = true;
        }

        if (it->sameMap) {
            it->Spawn(true, false, { 1, 1, 1 });
        }
    }

    while (this->runThread || !this->pauseThread) {
        {
            std::unique_lock<std::mutex> lck(mutex); //Wait for the sesion to restart
            waitForPaused.wait(lck, [] { return (networkGhostPlayer->runThread) ? !networkGhostPlayer->pauseThread.load() : true; });
        }
        if (!this->runThread) { //If needs to stop the thread (game quit, disconnect)
            return;
        }

        //Send our position to server
        this->UpdatePlayer();

        //Update other players
        sf::Packet data_packet;
        if (!this->ReceiveNetworkData(data_packet, 30)) {
            this->Disconnect(true);
            console->Warning("Connection to the server lost ! You're now disconnected !\n");
            return;
        }

        NetworkDataPlayer data;
        data_packet >> data;

        if (data.header == HEADER::UPDATE) { //Received new pos/ang or echo of our update
            if (data.ip != this->ip_client) {
                auto ghost = this->GetGhostByID(data.ip);
                if (ghost != nullptr) {
                    ghost->currentMap = data.dataGhost.currentMap;

                    if (ghost->currentMap != engine->m_szLevelName) { //If on a different map
                        ghost->sameMap = false;
                    } else if (ghost->currentMap == engine->m_szLevelName && !ghost->sameMap) { //If previously on a different map but now on the same one
                        ghost->sameMap = true;
                        ghost->Spawn(true, false, QAngleToVector(data.dataGhost.position));
                    }

                    if (ghost->sameMap && !pauseThread) { //" && !pauseThread" to verify the map is still loaded
                        this->SetPosAng(data.ip, QAngleToVector(data.dataGhost.position), QAngleToVector(data.dataGhost.view_angle));
                    }
                }
            }
        } else if (data.header == HEADER::PING) {
            auto stop = this->clock.now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - this->start);
            console->Print("Ping returned in %lld ms\n", elapsed.count());
        }

        if (!data.message.empty()) {
            console->Print(data.message.c_str());
        }
    }

    this->ghostPool.clear();
}

void NetworkGhostPlayer::CheckConnection()
{
    sf::SocketSelector tcpSelector;
    tcpSelector.add(this->tcpSocket);
    while (this->runThread) {
        sf::Packet packet;
        if (tcpSelector.wait(sf::milliseconds(500))) {
            this->tcpSocket.receive(packet);
            NetworkDataPlayer data;
            packet >> data;
            if (data.header == HEADER::CONNECT) {
                this->ghostPool.push_back(this->SetupGhost(data));
                if (this->runThread) {
                    auto ghost = this->GetGhostByID(data.ip);
                    if (ghost->sameMap) {
                        ghost->Spawn(true, false, { 1, 1, 1 });
                    }
                }
                console->Print("Player %s has connected !\n", data.name.c_str());
            } else if (data.header == HEADER::DISCONNECT) {
                int id = 0;
                for (; id < this->ghostPool.size(); ++id) {
                    if (this->ghostPool[id]->ID == data.ip) {
                        break;
                    }
                    this->ghostPool[id]->Stop();
                    this->ghostPool.erase(this->ghostPool.begin() + id);
                }
            }
        }
    }
}

GhostEntity* NetworkGhostPlayer::SetupGhost(NetworkDataPlayer& data)
{
    GhostEntity* tmp_ghost = new GhostEntity;
    tmp_ghost->name = data.name;
    tmp_ghost->ID = data.ip;
    tmp_ghost->currentMap = data.dataGhost.currentMap;
    tmp_ghost->sameMap = (data.dataGhost.currentMap == engine->m_szLevelName);
    return tmp_ghost;
}

void NetworkGhostPlayer::UpdatePlayer()
{
    NetworkDataPlayer data_client = this->CreateNetworkData();
    data_client.dataGhost = this->GetPlayerData();
    data_client.header = HEADER::UPDATE;
    this->SendNetworkData(data_client);
}

//Commands

CON_COMMAND(sar_ghost_connect_to_server, "Connect to the server : <ip address> <port> [local] :\n"
                                         "ex: 'localhost 53000' - '127.0.0.1 53000' - 89.10.20.20 53000'.")
{
    if (args.ArgC() <= 2) {
        console->Print(sar_ghost_connect_to_server.ThisPtr()->m_pszHelpString);
        return;
    }

    if (networkGhostPlayer->IsConnected()) {
        console->Warning("Already connected to the server !\n");
        return;
    }

    if (args.ArgC() == 4) {
        networkGhostPlayer->ip_client = sf::IpAddress::getLocalAddress();
    } else { //If extern
        networkGhostPlayer->ip_client = sf::IpAddress::getPublicAddress();
    }

    networkGhostPlayer->ConnectToServer(args[1], std::atoi(args[2]));
}

CON_COMMAND(sar_ghost_ping, "Send ping\n")
{

    sf::Packet packet;
    NetworkDataPlayer data = networkGhostPlayer->CreateNetworkData();
    data.header = HEADER::PING;
    packet << data;

    networkGhostPlayer->start = networkGhostPlayer->clock.now();
    networkGhostPlayer->socket.send(packet, networkGhostPlayer->ip_server, networkGhostPlayer->port_server);
}

CON_COMMAND(sar_ghost_disconnect, "Disconnect the player from the server\n")
{

    if (!networkGhostPlayer->IsConnected()) {
        console->Warning("You are not connected to a server !\n");
        return;
    }

    networkGhostPlayer->Disconnect(false);
    console->Print("You have successfully been disconnected !\n");
}

CON_COMMAND(sar_ghost_stop_server, "Stop the server\n")
{
    if (!networkGhostPlayer->IsConnected()) {
        console->Warning("You are not connected to a server !\n");
        return;
    }

    networkGhostPlayer->StopServer();
}

CON_COMMAND(sar_ghost_name, "Name that will be displayed\n")
{
    if (args.ArgC() <= 1) {
        console->Print(sar_ghost_name.ThisPtr()->m_pszHelpString);
        return;
    }
    networkGhostPlayer->name = args[1];
}
