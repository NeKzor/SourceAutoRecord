#include "NetworkGhostPlayer.hpp"

#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Utils/SDK.hpp"

#include "GhostPlayer.hpp"

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
    , tickrate(30)
    , isInLevel(false)
    , pausedByServer(false)
    , countdown(-1)
    , modelName("models/props/food_can/food_can_open.mdl")
{
    this->hasLoaded = true;
    this->socket.setBlocking(false);
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
    connection_packet << HEADER::CONNECT << this->socket.getLocalPort() << this->name << this->GetPlayerData() << std::string(engine->m_szLevelName) << this->modelName;
    tcpSocket.send(connection_packet);

    sf::SocketSelector tcpSelector;
    tcpSelector.add(tcpSocket);

    sf::Packet confirmation_packet;
    if (tcpSelector.wait(sf::seconds(30))) {
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
        std::string ghostModelName;
        confirmation_packet >> ID >> name >> data >> currentMap >> ghostModelName;
        this->ghostPool.push_back(this->SetupGhost(ID, name, data, currentMap, ghostModelName));
    }
    console->Print("Successfully connected to the server !\n%d player connected\n", nbPlayer);

    this->isConnected = true;
    this->isInLevel = true;
    this->StartThinking();
}

void NetworkGhostPlayer::Disconnect()
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
    this->isInLevel = false;
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
    this->isInLevel = false;
}

void NetworkGhostPlayer::Countdown(sf::Uint64 epoch, sf::Uint32 time)
{
    auto oldEpoch = std::chrono::milliseconds(epoch);
    auto t = std::chrono::seconds(time);
    auto now = std::chrono::system_clock::now();
    auto finalTime = (now + t); // Synch the clocks on the final time (works)
    //auto finalTime = (now + (now.time_since_epoch() - oldEpoch) + t); // Synch the clocks on the final time + network delay (actually add delay, I don't know why)
    auto startTime = finalTime - t + std::chrono::seconds(3); // Start the coutdown 3s after the command
    this->countdown = time;
    this->startCountdown = startTime;
}

void NetworkGhostPlayer::Countdown(sf::Uint64 epoch, sf::Uint32 time, QAngle position)
{
    this->teleportCountdown = position;
    this->Countdown(epoch, time);
}

bool NetworkGhostPlayer::IsConnected()
{
    return this->isConnected;
}

int NetworkGhostPlayer::ReceivePacket(sf::Packet& packet, sf::IpAddress& ip, int timeout)
{
    unsigned short port;

    /*if (selector.wait(sf::milliseconds(timeout))) {
        this->socket.receive(packet, ip, port);
    } else {
        packet << HEADER::NONE;
        if (!this->IsConnected()) {
            packet << "Error: Timeout reached ! You are now disconnected !\n";
            return -1; //Not connected anymore
        }

        return 0; //Connected but don't receive packets (ex: 1 player online)
    }

    return 1; //Received packet*/

    if (this->socket.receive(packet, ip, port) == sf::Socket::Done) {
        return 1;
    } else {
        return 0;
    }
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
    std::map<unsigned int, sf::Packet> packet_queue;

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
        int success = 1;
        if (this->selector.wait()) {
            while (success != 0) {
                sf::Packet packet;
                sf::IpAddress ip;
                success = this->ReceivePacket(packet, ip, 50);
                if (success == 1) {
                    packet_queue.insert({ ip.toInteger(), packet });
                }
            }
        }

        /*if (success == -1) {
            std::string message;
            data_packet >> message;
            console->Warning(message.c_str());
            this->Disconnect(true);
            return;
        } else {*/
        for (auto& data_packet : packet_queue) {
            HEADER header;
            data_packet.second >> header;
            if (header == HEADER::UPDATE) { //Received new pos/ang or echo of our update
                sf::Uint32 ID;
                DataGhost data;
                data_packet.second >> ID >> data;
                auto ghost = this->GetGhostByID(ID);
                if (ghost != nullptr) {
                    if (ghost->sameMap && !pauseThread) { //" && !pauseThread" to verify the map is still loaded
                        if (ghost->ghost_entity == nullptr) {
                            ghost->Spawn(true, false, QAngleToVector(data.position));
                        }
                        //this->SetPosAng(ID, QAngleToVector({ data.position.x, data.position.y, data.position.z + sar_ghost_height.GetFloat() }), QAngleToVector(data.view_angle));
                        ghost->oldPos = ghost->newPos;
                        ghost->newPos = { { data.position.x, data.position.y, data.position.z + sar_ghost_height.GetFloat() }, { data.view_angle.x, data.view_angle.y, data.view_angle.z } };
                        ghost->loopTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - ghost->lastUpdate).count();
                        ghost->lastUpdate = std::chrono::steady_clock::now();
                    }
                }
            }
        }
        packet_queue.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(this->tickrate));
    }

    this->ghostPool.clear();
}

void NetworkGhostPlayer::CheckConnection()
{
    sf::SocketSelector tcpSelector;
    tcpSelector.add(this->tcpSocket);
    while (this->runThread) {
        sf::Packet packet;
        if (tcpSelector.wait(sf::seconds(2))) {
            sf::Socket::Status status = this->tcpSocket.receive(packet);
            if (status == sf::Socket::Done) {
                HEADER header;
                packet >> header;
                if (header == HEADER::CONNECT) {
                    sf::Uint32 ID;
                    std::string name;
                    DataGhost data;
                    std::string currentMap;
                    std::string ghostModelName;
                    packet >> ID >> name >> data >> currentMap >> ghostModelName;
                    this->ghostPool.push_back(this->SetupGhost(ID, name, data, currentMap, ghostModelName));
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
                    console->Print("Player %s is now on %s\n", ghost->name.c_str(), newMap.c_str());
                    if (newMap == engine->m_szLevelName) {
                        ghost->sameMap = true;
                    } else {
                        ghost->sameMap = false;
                    }
                    ghost->currentMap = newMap;
                } else if (header == HEADER::MESSAGE) {
                    sf::Uint32 ID;
                    std::string message;
                    packet >> ID >> message;
                    std::string cmd = "say " + this->GetGhostByID(ID)->name + ": " + message;
                    engine->ExecuteCommand(cmd.c_str());
                } else if (header == HEADER::PING) {
                    auto stop = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - this->start);
                    console->Print("Ping returned in %lld ms\n", elapsed.count());
                } else if (header == HEADER::COUNTDOWN) {
                    sf::Uint64 epoch;
                    sf::Uint32 time;

                    packet >> epoch >> time;
                    this->Countdown(epoch, time);
                    this->shouldTeleportCountdown = false;
                } else if (header == HEADER::COUNTDOWN_AND_TELEPORT) {
                    sf::Uint64 epoch;
                    sf::Uint32 time;
                    float x, y, z;

                    packet >> epoch >> time >> x >> y >> z;
                    this->Countdown(epoch, time, { x, y, z });
                    this->shouldTeleportCountdown = true;
                }
            } else if (status == sf::Socket::Disconnected) {
                console->Warning("Connexion has been interrupted ! You have been disconnected !\n");
                this->Disconnect();
            }
        }
    }
}

GhostEntity* NetworkGhostPlayer::SetupGhost(sf::Uint32& ID, std::string name, DataGhost& data, std::string& currentMap, std::string& modelName)
{
    GhostEntity* tmp_ghost = new GhostEntity;
    tmp_ghost->name = name;
    tmp_ghost->ID = ID;
    tmp_ghost->currentMap = currentMap;
    tmp_ghost->sameMap = (currentMap == engine->m_szLevelName);
    tmp_ghost->ChangeModel(modelName);
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

void NetworkGhostPlayer::ClearGhosts()
{
    for (auto& ghost : this->ghostPool) {
        ghost->Stop();
    }
}

//Commands

CON_COMMAND(sar_ghost_connect_to_server, "Connect to the server : <ip address> <port> :\n"
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

    std::string ip = args[1];
    if (ip.find("10.") == 0 || ip.find("192.168.") == 0) { //If local
        networkGhostPlayer->ip_client = sf::IpAddress::getLocalAddress();
    } else {
        int local = false;
        for (int i = 16; i < 32; ++i) {
            if (ip.find("172." + std::to_string(i)) == 0) {
                networkGhostPlayer->ip_client = sf::IpAddress::getLocalAddress();
                local = true;
                break;
            }
        }
        if (local == false) {
            networkGhostPlayer->ip_client = sf::IpAddress::getPublicAddress();
        }
    }

    networkGhostPlayer->ConnectToServer(args[1], std::atoi(args[2]));
}

CON_COMMAND(sar_ghost_ping, "Send ping\n")
{
    sf::Packet packet;
    packet << HEADER::PING;

    networkGhostPlayer->start = std::chrono::steady_clock::now();
    networkGhostPlayer->tcpSocket.send(packet);
}

CON_COMMAND(sar_ghost_disconnect, "Disconnect the player from the server\n")
{

    if (!networkGhostPlayer->IsConnected()) {
        console->Warning("You are not connected to a server !\n");
        return;
    }

    networkGhostPlayer->Disconnect();
    console->Print("You have successfully been disconnected !\n");
}

/*CON_COMMAND(sar_ghost_stop_server, "Stop the server\n")
{
    if (!networkGhostPlayer->IsConnected()) {
        console->Warning("You are not connected to a server !\n");
        return;
    }

    networkGhostPlayer->StopServer();
}*/

CON_COMMAND(sar_ghost_name, "Name that will be displayed\n")
{
    if (args.ArgC() <= 1) {
        console->Print(sar_ghost_name.ThisPtr()->m_pszHelpString);
        return;
    }
    networkGhostPlayer->name = args[1];
}

CON_COMMAND(sar_ghost_tickrate, "Adjust the tickrate\n")
{
    if (args.ArgC() <= 1) {
        console->Print(sar_ghost_tickrate.ThisPtr()->m_pszHelpString);
        return;
    }
    networkGhostPlayer->tickrate = std::chrono::milliseconds(std::atoi(args[1]));
}

CON_COMMAND(sar_ghost_countdown, "Start a countdown\n")
{
    if (args.ArgC() < 2) {
        console->Print(sar_ghost_tickrate.ThisPtr()->m_pszHelpString);
        return;
    } else if (args.ArgC() >= 5) {
        sf::Packet packet;
        packet << HEADER::COUNTDOWN_AND_TELEPORT << sf::Uint32(std::atoi(args[1])) << std::stof(args[2]) << std::stof(args[3]) << std::stof(args[4]);
        networkGhostPlayer->tcpSocket.send(packet);
    } else if (args.ArgC() < 5) {
        sf::Packet packet;
        packet << HEADER::COUNTDOWN << sf::Uint32(std::atoi(args[1]));
        networkGhostPlayer->tcpSocket.send(packet);
    }
}

//Cause crash at sar_exit
/*CON_COMMAND(sar_ghost_message, "Send a message toother players\n")
{
    if (args.ArgC() <= 1) {
        console->Print(sar_ghost_message.ThisPtr()->m_pszHelpString);
        return;
    }
    sf::Packet packet;
    packet << HEADER::MESSAGE;
    std::string message = "";
    for (int i = 1; i < args.ArgC(); ++i) {
        message += args[i];
    }
    packet << message;
    networkGhostPlayer->tcpSocket.send(packet);
}*/
