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

NetworkGhostPlayer* networkGhostPlayer;

std::mutex mutex;

NetworkGhostPlayer::NetworkGhostPlayer()
    : ip_server("localhost")
    , port_server(53000)
    , name("FrenchSaves10Ticks")
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

    sf::Packet connection_packet;
    connection_packet << HEADER::CONNECT << this->socket.getLocalPort() << this->name << this->GetPlayerData() << std::string(engine->m_szLevelName);
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

    //Add every player connected to the ghostPool
    sf::Uint32 nbPlayer;
    confirmation_packet >> nbPlayer;
    for (sf::Uint32 i = 0; i < nbPlayer; ++i) {
        sf::Uint32 ID;
        std::string name;
        DataGhost data;
        std::string currentMap;
        confirmation_packet >> ID >> name >> data >> currentMap;
        this->ghostPool.push_back(this->SetupGhost(ID, name, data, currentMap));
    }

    console->Print("Successfully connected to the server !\n%d player connected\n", nbPlayer);

    this->isConnected = true;
    this->StartThinking();
}

void NetworkGhostPlayer::Disconnect(bool forced)
{
    this->runThread = false;
    this->waitForPaused.notify_one(); //runThread being false will make the thread stopping no matter if pauseThread is true or false

    sf::Packet packet;
    packet << HEADER::DISCONNECT;
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

    sf::Packet packet;
    packet << HEADER::STOP_SERVER;
    this->tcpSocket.send(packet);

    HEADER header = HEADER::NONE;
    this->tcpSocket.setBlocking(true);
    while (header != HEADER::STOP_SERVER) {
        sf::Packet confirmation_packet;
        this->tcpSocket.receive(confirmation_packet);
        confirmation_packet >> header;
    }

    console->Print("Server will stop !\n");

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
    return (this->tcpSocket.getRemoteAddress() == sf::IpAddress::None) ? false : true;
}

int NetworkGhostPlayer::ReceivePacket(sf::Packet& packet, int timeout)
{
    sf::IpAddress ip;
    unsigned short port;

    if (selector.wait(sf::seconds(timeout))) {
        this->socket.receive(packet, ip, port);
    } else {
        packet << HEADER::NONE;
        if (!this->IsConnected()) {
            packet << "Error: Timeout reached ! You are now disconnected !\n";
            return -1; //Not connected anymore
        }

        return 0; //Connected but don't receive packets (ex: 1 player online)
    }

    return 1; //Received packet
}

DataGhost NetworkGhostPlayer::GetPlayerData()
{
    DataGhost data = {
        VectorToQAngle(server->GetAbsOrigin(server->GetPlayer(GET_SLOT() + 1))),
        server->GetAbsAngles(server->GetPlayer(GET_SLOT() + 1))
    };

    return data;
}

GhostEntity* NetworkGhostPlayer::GetGhostByID(const sf::Uint32& ID)
{
    for (auto it : this->ghostPool) {
        if (it->ID == ID) {
            return it;
        }
    }
    return nullptr;
}

void NetworkGhostPlayer::SetPosAng(sf::Uint32& ID, Vector position, Vector angle)
{
    this->GetGhostByID(ID)->SetPosAng(position, angle);
}

//Update other players
void NetworkGhostPlayer::UpdateGhostsCurrentMap()
{
    this->GetGhostByID(this->ip_client.toInteger())->currentMap = engine->m_szLevelName;
    for (auto& it : this->ghostPool) {
        if (engine->m_szLevelName == it->currentMap) {
            it->sameMap = true;
        } else {
            it->sameMap = false;
        }
    }
}

//Notify network of map change
void NetworkGhostPlayer::UpdateCurrentMap()
{
    sf::Packet packet;
    packet << HEADER::MAP_CHANGE << std::string(engine->m_szLevelName);
    this->tcpSocket.send(packet);
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
            std::unique_lock<std::mutex> lck(mutex); //Wait for the session to restart
            waitForPaused.wait(lck, [] { return (networkGhostPlayer->runThread) ? !networkGhostPlayer->pauseThread.load() : true; });
        }
        if (!this->runThread) { //If needs to stop the thread (game quit, disconnect)
            return;
        }

        //Send our position to server
        this->UpdatePlayer();

        //Update other players
        sf::Packet data_packet;
        int success = this->ReceivePacket(data_packet, 30);
        if (success == -1) {
            std::string message;
            data_packet >> message;
            console->Warning(message.c_str());
            this->Disconnect(true);
            return;
        } else if (success == 1) {
            HEADER header;
            data_packet >> header;
            if (header == HEADER::UPDATE) { //Received new pos/ang or echo of our update
                sf::Uint32 ID;
                DataGhost data;
                data_packet >> ID >> data;
                auto ghost = this->GetGhostByID(ID);
                if (ID != this->ip_client.toInteger()) { //Remember ID == IP.toInteger
                    if (ghost != nullptr) {
                        if (ghost->sameMap && !pauseThread) { //" && !pauseThread" to verify the map is still loaded
                            if (ghost->ghost_entity == nullptr) {
                                ghost->Spawn(true, false, QAngleToVector(data.position));
                            }
                            this->SetPosAng(ID, QAngleToVector(data.position), QAngleToVector(data.view_angle));
                        }
                    }
                }
            } else if (header == HEADER::PING) {
                auto stop = this->clock.now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - this->start);
                console->Print("Ping returned in %lld ms\n", elapsed.count());
            }
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
            HEADER header;
            packet >> header;
            if (header == HEADER::CONNECT) {
                sf::Uint32 ID;
                std::string name;
                DataGhost data;
                std::string currentMap;
                packet >> ID >> name >> data >> currentMap;
                this->ghostPool.push_back(this->SetupGhost(ID, name, data, currentMap));
                if (this->runThread) {
                    auto ghost = this->GetGhostByID(ID);
                    if (ghost->sameMap) {
                        ghost->Spawn(true, false, { 1, 1, 1 });
                    }
                }
                console->Print("Player %s has connected !\n", name.c_str());
            } else if (header == HEADER::DISCONNECT) {
                sf::Uint32 ID;
                packet >> ID;
                int id = 0;
                for (; id < this->ghostPool.size(); ++id) {
                    if (this->ghostPool[id]->ID == ID) {
                        break;
                    }
                    this->ghostPool[id]->Stop();
                    this->ghostPool.erase(this->ghostPool.begin() + id);
                }
            } else if (header == HEADER::STOP_SERVER) {
                this->StopServer();
            } else if (header == HEADER::MAP_CHANGE) {
                sf::Uint32 ID;
                std::string newMap;
                packet >> ID >> newMap;
                auto ghost = this->GetGhostByID(ID);
                if (newMap == engine->m_szLevelName) {
                    ghost->sameMap = true;
                } else {
                    ghost->sameMap = false;
                }
                ghost->currentMap = newMap;
            }
        }
    }
}

GhostEntity* NetworkGhostPlayer::SetupGhost(sf::Uint32& ID, std::string name, DataGhost& data, std::string& currentMap)
{
    GhostEntity* tmp_ghost = new GhostEntity;
    tmp_ghost->name = name;
    tmp_ghost->ID = ID;
    tmp_ghost->currentMap = currentMap;
    tmp_ghost->sameMap = (currentMap == engine->m_szLevelName);
    return tmp_ghost;
}

void NetworkGhostPlayer::UpdatePlayer()
{
    HEADER header = HEADER::UPDATE;
    DataGhost dataGhost = this->GetPlayerData();
    sf::Packet packet;
    packet << header << dataGhost;
    this->socket.send(packet, this->ip_server, this->port_server);
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
    packet << HEADER::PING;

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
