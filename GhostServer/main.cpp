#include "NetworkManager.h"
#include <SFML/Network.hpp>
#include <TGUI/TGUI.hpp>

enum class COMMANDTYPE {
    PING,
    CONNECT,
    DISCONNECT,
    STOP_SERVER,
    MESSAGE,
    COUNTDOWN,
    PRE_COUNTDOWN,
    POST_COUNTDOWN,
    FONTSIZE,
    HELP
};

struct Command {
    COMMANDTYPE commandType;
    std::string helpString;
};

std::map<std::string, Command> commandList;

std::string LowerString(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

std::string HandleCommand(std::string input, NetworkManager& network, tgui::EditBox::Ptr& edit, tgui::TextBox::Ptr& log)
{
    if (input.empty()) {
        return "";
    }

    //Getting the args
    std::vector<std::string> args;
    std::stringstream check(input);
    std::string tmp;
    while (std::getline(check, tmp, ' ')) {
        args.push_back(LowerString(tmp));
    }
    auto command = commandList.find(args[0]);

    if (command->second.commandType == COMMANDTYPE::STOP_SERVER) {
        network.StopServer();
        return "Server wil stop !";
    } else if (command->second.commandType == COMMANDTYPE::DISCONNECT) {
        if (args.size() < 2) {
            return "Not enough argument -> " + command->second.helpString;
        }

        sf::IpAddress ip = args[1];
        std::string name = network.Disconnect(ip.toInteger());
        if (name.empty()) {
            return "No player corresponding to the ip !";
        }
    } else if (command->second.commandType == COMMANDTYPE::PRE_COUNTDOWN) {
        std::string commands = ""; //If there's multiple commands
        for (int i = 1; i < args.size(); ++i) {
            commands += args[i] + " ";
        }
        network.SetCommandPreCoutdown(commands);
        return "";
    } else if (command->second.commandType == COMMANDTYPE::POST_COUNTDOWN) {
        std::string commands = ""; //If there's multiple commands
        for (int i = 1; i < args.size(); ++i) {
            commands += args[i] + " ";
        }
        network.SetCommandPostCoutdown(commands);
        return "";
    } else if (command->second.commandType == COMMANDTYPE::COUNTDOWN) {
        if (args.size() < 2) {
            return "Not enough argument -> " + command->second.helpString;
        }
        network.StartCountdown(std::stoi(args[1]));
        return "Countdown of " + args[1] + " started !";
    } else if (command->second.commandType == COMMANDTYPE::FONTSIZE) {
        if (args.size() < 2) {
            return "Not enough argument -> " + command->second.helpString;
        }
        log->setTextSize(std::stoi(args[1]));
        edit->setTextSize(std::stoi(args[1]));
        return "";
    } else if (command->second.commandType == COMMANDTYPE::HELP) {
        if (args.size() == 1) { //Print every help strings
            std::string helpString;
            for (auto& c : commandList) {
                helpString += c.second.helpString + "\n";
            }
            return helpString;
        } else {
            return commandList.find(args[1])->second.helpString;
        }
    }

    return "\"" + args[0] + "\" is not an existing command. Type \"help\" to see all the available commands";
}

void HandleEvent(tgui::TextBox::Ptr& log, std::vector<sf::Packet>& e, NetworkManager& network)
{
    for (auto& it : e) {
        HEADER header;
        std::string name;
        it >> header >> name;
        if (header == HEADER::CONNECT) {
            log->addText("Player " + name + " has connected !\n");
        } else if (header == HEADER::DISCONNECT) {
            log->addText("Player " + name + " has disconnected !\n");
        } else if (header == HEADER::MESSAGE) {
            std::string message;
            it >> message;
            log->addText(name + " : \"" + message + "\"\n");
        } else if (header == HEADER::COUNTDOWN) {
            log->addText("Player " + name + " has started a countdown !\n");
        }
    }
}

void commandEntered(tgui::EditBox::Ptr& edit, tgui::TextBox::Ptr& log, NetworkManager& network)
{
    std::string text = edit->getText();
    log->addText(text + "\n");
    log->addText(HandleCommand(text, network, edit, log) + "\n");
    edit->setText("");
}

int main()
{
    commandList.insert({ "stopserver", { COMMANDTYPE::STOP_SERVER, "stopserver : Disconnects all the players and stops the server" } });
    commandList.insert({ "disconnect", { COMMANDTYPE::DISCONNECT, "disconnect <ip> : Disconnects the player specified" } });
    commandList.insert({ "setprecommand", { COMMANDTYPE::PRE_COUNTDOWN, "setprecommand <command> : Command that will be executed at the beggining of the countdown" } });
    commandList.insert({ "setpostcommand", { COMMANDTYPE::POST_COUNTDOWN, "setpostcommand <command> : Command that will be executed at the end of the countdown" } });
    commandList.insert({ "countdown", { COMMANDTYPE::COUNTDOWN, "countdown <time> : Starts a countdown for all the players. Will use setprecommand and setpostcommand is those were used" } });
    commandList.insert({ "fontsize", { COMMANDTYPE::FONTSIZE, "fontsize <size> : Change the size of the font in the console" } });
    commandList.insert({ "help", { COMMANDTYPE::HELP, "help [command] : Prints help string either of the specifed command or all the commands" } });

    sf::RenderWindow window{
        { 800, 600 }, "Ghost Server"
    };
    window.setFramerateLimit(60);
    tgui::Gui gui{ window };
    gui.loadWidgetsFromFile("Server.txt");
    gui.setFont("Consolas.ttf");

    tgui::TextBox::Ptr log = gui.get<tgui::TextBox>("log");
    log->setTextSize(16);

    NetworkManager network(53000);
    if (!network.IsConnected()) {
        log->addText("Error : Can't connect with " + network.GetPublicIP().toString() + " : " + std::to_string(network.GetPort()) + " !\n");
        window.close();
        return 0;
    }

    log->addText("Server started on <" + network.GetPublicIP().toString() + "> (Local : " + network.GetLocalIP().toString() + ") ; Port: " + std::to_string(network.GetPort()) + ">\n");

    tgui::EditBox::Ptr commandEdit = gui.get<tgui::EditBox>("commandEdit");
    commandEdit->connect("ReturnKeyPressed", commandEntered, std::ref(commandEdit), std::ref(log), std::ref(network));

    std::vector<sf::Packet> e;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                network.StopServer();
                window.close();
            }
            gui.handleEvent(event);
        }
        if (network.IsStopped()) {
            window.close();
        }
        network.GetEvent(e);
        if (e.size() > 0) {
            HandleEvent(log, e, network);
        }

        window.clear();
        gui.draw();
        window.display();
    }

    return 0;
}
