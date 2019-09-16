#include "NetworkGhostPlayer.hpp"

#include "GhostPlayer.hpp"
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
    , networkGhosts()
    , runThread(false)
    , isConnected(false)
    , networkThread()
    , connectThread()
    , disconnectThread()
{
    this->hasLoaded = true;
    //this->ip_client = sf::IpAddress::getPublicAddress();
    this->ip_client = "localhost"; //Remove that after tests
    socket.bind(sf::Socket::AnyPort, ip_client);
    socket.setBlocking(false);
}

NetworkDataPlayer NetworkGhostPlayer::CreateNetworkData()
{
    NetworkDataPlayer networkData{ HEADER::NONE, "Blenderiste09", this->ip_client.toString(), this->socket.getLocalPort() };
    DataGhost dataGhost{ { 0, 0, 0 }, { 0, 0, 0 } };
    networkData.dataGhost = dataGhost;

    return networkData;
}

void NetworkGhostPlayer::ConnectToServer(sf::IpAddress ip, unsigned short port)
{
    this->ip_server = ip;
    this->port_server = port;
    NetworkDataPlayer data = this->CreateNetworkData();
    data.header = HEADER::CONNECT;
    this->SendNetworkData(data);

    NetworkDataPlayer confirmation = this->ReceiveNetworkData();

    if (!confirmation.message.empty()) {
        if (confirmation.message == "Error: Timeout reached ! Can't connect to the server !\n" || confirmation.message == "Error: Connexion to the server lost !\n") {
            console->Warning(confirmation.message.c_str());
            return;
        } else {
            console->Print(confirmation.message.c_str());
        }

        this->runThread = true;
        this->isConnected = true;
        this->networkThread = std::thread(&NetworkGhostPlayer::NetworkThink, this, std::ref(this->runThread));
        this->networkThread.detach();
    }
}

void NetworkGhostPlayer::Disconnect(bool forced)
{
    NetworkDataPlayer data = this->CreateNetworkData();
    data.header = HEADER::DISCONNECT;
    this->SendNetworkData(data);

    this->runThread = false;

    if (!forced) {
        NetworkDataPlayer confirmation = this->ReceiveNetworkData();
        console->Print(confirmation.message.c_str());
    }
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

NetworkDataPlayer NetworkGhostPlayer::ReceiveNetworkData()
{
    sf::Packet packet;
    sf::IpAddress ip;
    unsigned short port;

    std::chrono::time_point<std::chrono::steady_clock> start, end;
    start = std::chrono::steady_clock::now();
    end = start + std::chrono::seconds(10);
    bool ok = false;

    while (std::chrono::steady_clock::now() < end && !ok) {
        if (this->socket.receive(packet, ip, port) == sf::Socket::Done) {
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

        return data;
    }

    NetworkDataPlayer data;
    packet >> data;
    return data;
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
    std::strcpy(data.currentMap, engine->m_szLevelName);

    return data;
}

//Called every ticks on another thread
void NetworkGhostPlayer::NetworkThink(bool& run)
{
    while (run) {
        //Send our position to server
        NetworkDataPlayer data_client = this->CreateNetworkData();
        data_client.dataGhost = this->GetPlayerData();
        data_client.header = HEADER::UPDATE;
        this->SendNetworkData(data_client);

        //Update other players
        NetworkDataPlayer data = this->ReceiveNetworkData();
        switch (data.header) {
        case HEADER::CONNECT: //New player or echo of our connection
            if (data.ip != this->ip_client) {
                {
                    GhostEntity* ghost = new GhostEntity;
                    ghost->name = data.name;
                    ghost->ID = data.ip;
                    ghostPlayer->AddGhost(ghost);
                }
            }
            break;
        case HEADER::UPDATE: //Received new pos/ang or echo of our update
            //if (data.ip != this->ip_client) {
            if (data.ip == this->ip_client) {
                data.ip = "0";
            }
            if (!std::strcmp(ghostPlayer->GetGhostFromID(data.ip)->currentMap, engine->m_szLevelName)) {
                //If on a different map
                ghostPlayer->GetGhostFromID(data.ip)->sameMap = false;
            } else if (std::strcmp(ghostPlayer->GetGhostFromID(data.ip)->currentMap, engine->m_szLevelName) && !ghostPlayer->GetGhostFromID(data.ip)->sameMap) {
                //If previously on a different map but now on the same one
                ghostPlayer->GetGhostFromID(data.ip)->sameMap = true;
                ghostPlayer->GetGhostFromID(data.ip)->Spawn(true, false, QAngleToVector(data.dataGhost.position));
            }

            if (ghostPlayer->GetGhostFromID(data.ip)->sameMap) {
                ghostPlayer->SetPosAng(data.ip, QAngleToVector(data.dataGhost.position), QAngleToVector(data.dataGhost.view_angle));
            }
            //}
            break;
        case HEADER::DISCONNECT: //Ask for disconnection
            if (data.ip != this->ip_client) {
                console->Print("%s has disconnected !\n", data.name);
                ghostPlayer->StopByID(data.ip);
            }
            break;
        case HEADER::STOP_SERVER:
            ghostPlayer->StopAll();
            break;
        default:
            break;
        }

        if (!data.message.empty()) {
            console->Print(data.message.c_str());
        }
    }
}

/*void NetworkGhostPlayer::Run()
{
    while (this->runThread) {
        this->NetworkThink();
    }
}

void NetworkGhostPlayer::Run(bool force)
{
    if (force) {
        this->NetworkThink();
    }
}*/

//Commands

CON_COMMAND(sar_ghost_connect_to_server, "Connect to the server : <ip address> <port> :\n"
                                         "ex: 'localhost 53000' - '127.0.0.1 53000' - 89.10.20.20 53000'.")
{
    if (args.ArgC() <= 2) {
        console->Print(sar_ghost_connect_to_server.ThisPtr()->m_pszHelpString);
    }
    GhostEntity* ghost = new GhostEntity;
    ghost->name = "Blenderiste09";
    ghost->ID = "0";
    ghostPlayer->AddGhost(ghost);

    ghostPlayer->StopAll();
    ghostPlayer->isNetworking = true;
    networkGhostPlayer->connectThread = std::thread(&NetworkGhostPlayer::ConnectToServer, networkGhostPlayer, args[1], std::atoi(args[2]));
    networkGhostPlayer->connectThread.detach();

    /*sf::Thread thread([args]() {
        networkGhostPlayer->ConnectToServer(args[1], std::atoi(args[2]));
    });
    thread.launch();*/
    //networkGhostPlayer->ConnectToServer(args[1], std::atoi(args[2]));
}

CON_COMMAND(sar_ghost_send, "Send data player\n")
{

    NetworkDataPlayer data = networkGhostPlayer->CreateNetworkData();
    data.header = HEADER::CONNECT;
    data.dataGhost = networkGhostPlayer->GetPlayerData();
    networkGhostPlayer->SendNetworkData(data);
    data.header = HEADER::UPDATE;
    networkGhostPlayer->SendNetworkData(data);

    NetworkDataPlayer data_server = networkGhostPlayer->ReceiveNetworkData();
    console->Print("Received : from %s on port %d -->\n"
                   "Player name : %s\n"
                   "Position : %.3f %.3f %.3f\n"
                   "View angle : %.3f %.3f %.3f\n",
        data_server.ip.c_str(), data_server.port, data_server.name.c_str(), data_server.dataGhost.position.x, data_server.dataGhost.position.y, data_server.dataGhost.position.z, data_server.dataGhost.view_angle.x, data_server.dataGhost.view_angle.y, data_server.dataGhost.view_angle.z);
}

CON_COMMAND(sar_ghost_disconnect, "Disconnect the player from the server\n")
{
    networkGhostPlayer->disconnectThread = std::thread(&NetworkGhostPlayer::Disconnect, networkGhostPlayer, false);
    networkGhostPlayer->disconnectThread.detach();
}

CON_COMMAND(sar_ghost_stop_server, "Stop the server\n")
{
    networkGhostPlayer->StopServer();
}
