#include "VGui.hpp"

#include <algorithm>

#include "Features/Hud/Hud.hpp"
#include "Features/Session.hpp"
#include "Features/Timer/PauseTimer.hpp"

#include "Modules/Engine.hpp"
#include "Modules/Surface.hpp"

#include "SAR.hpp"

void VGui::Draw(Hud* const& hud)
{
    if (hud->ShouldDraw()) {
        hud->Paint(this->context.slot);
    }
}
void VGui::Draw(HudElement* const& element)
{
    if (element->ShouldDraw()) {
        element->Paint(&this->context);
    }
}

// CEngineVGui::Paint
DETOUR(Paint, PaintMode_t mode)
{
    auto result = Paint(thisptr, mode);

    surface->StartDrawing(surface->matsurface->ThisPtr());

    auto ctx = &vgui->context;

    ctx->Reset(GET_SLOT());

    if (ctx->slot == 0) {
        if (mode & PAINT_UIPANELS) {
            for (auto const& hud : vgui->huds) {
                vgui->Draw(hud);
            }

            for (auto const& element : vgui->elements) {
                vgui->Draw(element);
            }
        }
    } else if (ctx->slot == 1) {
        for (auto const& hud : vgui->huds) {
            if (hud->drawSecondSplitScreen) {
                vgui->Draw(hud);
            }
        }

        for (auto const& element : vgui->elements) {
            if (element->drawSecondSplitScreen) {
                vgui->Draw(element);
            }
        }
    }

    surface->FinishDrawing();

    return result;
}

void VGui::Init()
{
    this->enginevgui = Interface::Hookable(this, "VEngineVGui0");
    this->enginevgui->Hook(&hkPaint, Offsets::Paint);

    for (auto& hud : Hud::GetList()) {
        if (hud->version == SourceGame_Unknown || sar.game->Is(hud->version)) {
            this->huds.push_back(hud);
        }
    }

    HudElement::IndexAll();

    for (auto const& element : HudElement::GetList()) {
        if (element->version == SourceGame_Unknown || sar.game->Is(element->version)) {
            this->elements.push_back(element);
        }
    }

    std::sort(this->elements.begin(), this->elements.end(), [](const HudElement* a, const HudElement* b) {
        return a->orderIndex < b->orderIndex;
    });
}
void VGui::Shutdown()
{
    Interface::Destroy(this->enginevgui);
    this->huds.clear();
    this->elements.clear();
}

VGui* vgui;
