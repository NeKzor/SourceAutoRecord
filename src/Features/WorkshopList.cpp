#include "WorkshopList.hpp"

#include <cstring>
#if _WIN32
#include <filesystem>
#define fs std::filesystem
#else
#include <experimental/filesystem>
#define fs std::experimental::filesystem
#endif
#include <stdlib.h>

#include "Modules/Engine.hpp"

#include "Command.hpp"
#include "Utils.hpp"

WorkshopList* workshop;

WorkshopList::WorkshopList()
    : maps()
    , path(std::string(engine->GetGameDirectory()) + std::string("/maps/workshop"))
{
}
int WorkshopList::Update()
{
    auto before = this->maps.size();
    this->maps.clear();

    auto path = this->path;
    auto index = path.length() + 1;

    // Scan through all directories and find the map file
    for (auto& dir : fs::recursive_directory_iterator(path)) {
        if (dir.status().type() == fs::file_type::directory) {
            auto curdir = dir.path().string();
            for (auto& dirdir : fs::directory_iterator(curdir)) {
                auto file = dirdir.path().string();
                if (Utils::EndsWith(file, std::string(".bsp"))) {
                    auto map = file.substr(index);
                    map = map.substr(0, map.length() - 4);
                    this->maps.push_back(map);
                    break;
                }
            }
        }
    }

    return std::abs((int)before - (int)this->maps.size());
}

// Completion Function

DECL_COMMAND_COMPLETION(sar_workshop)
{
    if (workshop->maps.empty()) {
        workshop->Update();
    }

    for (auto& map : workshop->maps) {
        if (items.size() == COMMAND_COMPLETION_MAXITEMS) {
            break;
        }

        if (std::strlen(match) != std::strlen(cmd)) {
            if (std::strstr(map.c_str(), match)) {
                items.push_back(map);
            }
        } else {
            items.push_back(map);
        }
    }

    FINISH_COMMAND_COMPLETION();
}

// Commands

CON_COMMAND_FU_COMPLETION(sar_workshop,
    "Same as \"map\" command but lists workshop maps.\n"
    "Usage: sar_workshop <file> [ss/changelevel]\n",
    0, sar_workshop_CompletionFunc, SourceGame_Portal2 | SourceGame_ApertureTag)
{
    if (args.ArgC() < 2) {
        return console->Print(sar_workshop.ThisPtr()->m_pszHelpString);
    }

    auto command = std::string("map");

    if (args.ArgC() > 2) {
        if (!std::strcmp(args[2], "ss")) {
            command = std::string("ss_map");
        } else if (!std::strcmp(args[2], "changelevel")) {
            command = std::string("changelevel");
        }
    }

    engine->ExecuteCommand((command + std::string(" workshop/") + std::string(args[1])).c_str());
}
CON_COMMAND_U(sar_workshop_update, "Updates the workshop map list.\n", SourceGame_Portal2 | SourceGame_ApertureTag)
{
    console->Print("Added or removed %i map(s) to or from the list.\n", workshop->Update());
}
CON_COMMAND_U(sar_workshop_list, "Prints all workshop maps.\n", SourceGame_Portal2 | SourceGame_ApertureTag)
{
    if (workshop->maps.empty()) {
        workshop->Update();
    }

    for (const auto& map : workshop->maps) {
        console->Print("%s\n", map.c_str());
    }
}
