#include "NetworkManager.h"
#include <SFML/Network.hpp>
#include <TGUI/TGUI.hpp>

#include <iostream>

int main()
{
    sf::RenderWindow window{
        { 800, 600 }, "Ghost Server"
    };
    tgui::Gui gui{ window };
    gui.loadWidgetsFromFile("Server.txt");

    NetworkManager network(53000);
    if (!network.IsConnected()) {
        std::cout << "Error : Can't connect with " << network.GetPublicIP() << " : " << network.GetPort() << " !" << std::endl;
        return 0;
    }

    //std::cout << "Server started on <" << network.GetPublicIP() << "> (Local : " << network.GetLocalIP() << ") ; Port: " << network.GetPort() << ">" << std::endl;

	tgui::TextBox::Ptr log = gui.get<tgui::TextBox>("log");
    log->addText("Server started on <" + network.GetPublicIP().toString() + "> (Local : " + network.GetLocalIP().toString() + ") ; Port: " + std::to_string(network.GetPort()) + ">\n");
	
	tgui::EditBox::Ptr commandEdit = gui.get<tgui::EditBox>("commandEdit");
    commandEdit->connect("ReturnKeyPressed", [&]() { log->addText(commandEdit->getText() + "\n"); commandEdit->setText(""); });

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            gui.handleEvent(event);
        }

        window.clear();
        gui.draw();
        window.display();
    }


    bool runServer = true;
    network.StopServer(runServer);
    while (runServer) {
    
	}

    return 0;
}
