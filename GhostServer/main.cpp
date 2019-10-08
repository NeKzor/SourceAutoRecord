#include <SFML/Network.hpp>
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

//DataGhost

sf::Packet& operator>>(sf::Packet& packet, QAngle& angle)
{
    return packet >> angle.x >> angle.y >> angle.z;
}


#include <iostream>

int main()
{

    NetworkManager network(53000);
    if (!network.IsConnected()) {
        std::cout << "Error : Can't connect with " << network.GetPublicIP() << " : " << network.GetPort() << " !" << std::endl;
        return 0;
    }



            }
        }

    }


    bool runServer = true;
    network.StopServer(runServer);
    while (runServer) {
    
	}

    return 0;
}
