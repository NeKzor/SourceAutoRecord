#pragma once
#include <vector>

#include "Features/Hud/Hud.hpp"

#include "Module.hpp"

#include "Interface.hpp"
#include "Utils.hpp"

class VGui : public Module {
public:
    Interface* enginevgui = nullptr;

    HudContext context = HudContext();
    std::vector<Hud*> huds = std::vector<Hud*>();
    std::vector<HudElement*> elements = std::vector<HudElement*>();

public:
    void Draw(Hud* const& hud);
    void Draw(HudElement* const& element);

    VGui()
        : Module(MODULE("engine"))
    {
        this->isHookable = true;
    }

    void Init() override;
    void Shutdown() override;
};

extern VGui* vgui;
