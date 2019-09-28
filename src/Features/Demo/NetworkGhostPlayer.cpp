#include "NetworkGhostPlayer.hpp"

#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
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

NetworkGhostPlayer::NetworkGhostPlayer()
    : ip_server("localhost")
    , port_server(53000)
    , name("FrenchSaves10Ticks")
    , networkGhosts()
    , runThread(false)
    , isConnected(false)
    , networkThread()
    , ghostPool()
{
    this->hasLoaded = true;
    //this->ip_client = sf::IpAddress::getPublicAddress(); //TODO: handle automatically local network
    this->ip_client = sf::IpAddress::getLocalAddress();
    socket.bind(sf::Socket::AnyPort);
    selector.add(this->socket);
}

void NetworkGhostPlayer::ConnectToServer(std::string ip, unsigned short port)
{
    this->ip_server = ip;
    this->port_server = port;
    NetworkDataPlayer data = this->CreateNetworkData();
    data.header = HEADER::CONNECT;
    this->SendNetworkData(data);

    sf::Packet confirmation_packet;
    if (!this->ReceiveNetworkData(confirmation_packet, 5)) {
        console->Warning("Timeout reached ! Can't connect to the server %s:%d !\n", ip.c_str(), port);
        return;
    }

    NetworkDataPlayer confirmation;
    confirmation_packet >> confirmation;

    sf::Uint32 nbPlayer;
    confirmation_packet >> nbPlayer;
    for (sf::Uint32 i = 0; i < nbPlayer; ++i) {
        NetworkDataPlayer tmp_player;
        confirmation_packet >> tmp_player;
        this->ghostPool.push_back(this->SetupGhost(tmp_player));
    }

    if (!confirmation.message.empty()) {
        if (confirmation.message == "Error: Timeout reached ! Can't connect to the server !\n" || confirmation.message == "Error: Connexion to the server lost !\n") {
            console->Warning(confirmation.message.c_str());
            return;
        } else {
            console->Print(confirmation.message.c_str());
        }

        this->isConnected = true;
        this->StartThinking();
    }
}

void NetworkGhostPlayer::Disconnect(bool forced)
{
    NetworkDataPlayer data = this->CreateNetworkData();
    data.header = HEADER::DISCONNECT;
    this->SendNetworkData(data);

    this->runThread = false;

    if (!forced) { //If connection isn't lost
        sf::Packet confirmation_packet;
        bool ok = this->ReceiveNetworkData(confirmation_packet, 5);
        NetworkDataPlayer confirmation;
        confirmation_packet >> confirmation;
        console->Print(confirmation.message.c_str());
    }

    this->isConnected = false;
    this->selector.clear();
    this->socket.unbind();
}

void NetworkGhostPlayer::StopServer()
{
    NetworkDataPlayer data = this->CreateNetworkData();
    data.header = HEADER::STOP_SERVER;
    this->SendNetworkData(data);

    this->runThread = false;
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

    /*if (timeout > 0) {
        std::chrono::time_point<std::chrono::steady_clock> start, end;
        start = std::chrono::steady_clock::now();
        end = start + std::chrono::seconds(10);

        bool ok = false;
        while (std::chrono::steady_clock::now() < end && !ok) { //Timeout
            if (this->socket.receive(packet, ip, port) == sf::Socket::Done) { //TODO: Check ip and port ?
                ok = true;
            }
        }
        if (std::chrono::steady_clock::now() > end) {
            NetworkDataPlayer data;
            data.header = HEADER::NONE;
            if (this->IsConnected()) {
                data.message = "Error: Connexion to the server lost !\n";
                this->Disconnect(true);
            } else {
                data.message = "Error: Timeout reached ! Can't connect to the server !\n";
            }

            sf::Packet problem;
            problem << data;

            return problem;
        }
    } else {*/

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
    //}

    return true;
}

NetworkDataPlayer NetworkGhostPlayer::CreateNetworkData()
{
    NetworkDataPlayer networkData{ HEADER::NONE, this->name, this->ip_client.toString(), this->socket.getLocalPort() };
    DataGhost dataGhost{ { 0, 0, 0 }, { 0, 0, 0 }, engine->m_szLevelName };
    networkData.dataGhost = dataGhost;

    return networkData;
}

DataGhost NetworkGhostPlayer::GetPlayerData()
{
    QAngle pos = VectorToQAngle(server->GetAbsOrigin(server->GetPlayer(GET_SLOT() + 1)));
    pos.x += 64;
    DataGhost data = {
        //VectorToQAngle(server->GetAbsOrigin(server->GetPlayer(GET_SLOT() + 1))),
        pos,
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
    this->runThread = true;
    this->networkThread = std::thread(&NetworkGhostPlayer::NetworkThink, this, std::ref(this->runThread));
    this->networkThread.detach();
}

void NetworkGhostPlayer::StopThinking()
{
    this->runThread = false;
}

//Called on another thread
void NetworkGhostPlayer::NetworkThink(bool& run)
{
    for (auto& it : this->ghostPool) {
        if (it->sameMap) {
            it->Spawn(true, false, { 1, 1, 1 });
        }
    }

    while (run) {
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
        /*if (data.ip == this->ip_client) {
            return;
        }*/

        if (data.header == HEADER::CONNECT) { //New player or echo of our connection
            if (data.ip != this->ip_client) {
                this->ghostPool.push_back(this->SetupGhost(data));
            }
        } else if (data.header == HEADER::UPDATE) { //Received new pos/ang or echo of our update
            auto ghost = this->GetGhostByID(data.ip);
            if (ghost->currentMap != engine->m_szLevelName) { //If on a different map
                ghost->sameMap = false;
            } else if (ghost->currentMap == engine->m_szLevelName && !ghost->sameMap) { //If previously on a different map but now on the same one
                ghost->sameMap = true;
                ghost->Spawn(true, false, QAngleToVector(data.dataGhost.position));
            }

            if (ghost->sameMap && run) { //" && run" to verify the map is still loaded
                this->SetPosAng(data.ip, QAngleToVector(data.dataGhost.position), QAngleToVector(data.dataGhost.view_angle));
            }
        } else if (data.header == HEADER::DISCONNECT) { //Ask for disconnection
            if (data.ip != this->ip_client) {
                console->Print("%s has disconnected !\n", data.name);
                this->GetGhostByID(data.ip)->Stop();
            } else { //Confirmation of our disconnection
                this->socket.unbind();
                for (auto& it : this->ghostPool) {
                    it->Stop();
                }
            }
        } else if (data.header == HEADER::STOP_SERVER) {
            for (auto& it : this->ghostPool) {
                it->Stop();
            }
        }

        if (!data.message.empty()) {
            console->Print(data.message.c_str());
        }
    }
}

GhostEntity* NetworkGhostPlayer::SetupGhost(NetworkDataPlayer& data)
{
    GhostEntity* tmp_ghost = new GhostEntity;
    tmp_ghost->name = data.name;
    tmp_ghost->ID = data.ip;
    tmp_ghost->currentMap = data.dataGhost.currentMap;
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

    networkGhostPlayer->ConnectToServer(args[1], std::atoi(args[2]));
}

CON_COMMAND(sar_ghost_send, "Send data player\n")
{

    NetworkDataPlayer data = networkGhostPlayer->CreateNetworkData();
    data.header = HEADER::CONNECT;
    data.dataGhost = networkGhostPlayer->GetPlayerData();
    networkGhostPlayer->SendNetworkData(data);
    data.header = HEADER::UPDATE;
    networkGhostPlayer->SendNetworkData(data);

    sf::Packet data_server_packet;
    if (!networkGhostPlayer->ReceiveNetworkData(data_server_packet, 5)) {
        return;
    }

    NetworkDataPlayer data_server;
    data_server_packet >> data_server;

    console->Print("Received : from %s on port %d -->\n"
                   "Player name : %s\n"
                   "Position : %.3f %.3f %.3f\n"
                   "View angle : %.3f %.3f %.3f\n",
        data_server.ip.c_str(), data_server.port, data_server.name.c_str(), data_server.dataGhost.position.x, data_server.dataGhost.position.y, data_server.dataGhost.position.z, data_server.dataGhost.view_angle.x, data_server.dataGhost.view_angle.y, data_server.dataGhost.view_angle.z);
}

CON_COMMAND(sar_ghost_disconnect, "Disconnect the player from the server\n")
{

    if (!networkGhostPlayer->IsConnected()) {
        console->Warning("You are not connected to a server !\n");
        return;
    }

    networkGhostPlayer->Disconnect(false);
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
