#include "Crosshair.hpp"

#include "Variable.hpp"

#include "Modules/Surface.hpp"
#include "Modules/Engine.hpp"

Variable sar_crosshair("sar_crosshair", "0", "Enable custom crosshair.\n");
Variable cl_crosshair_t("cl_crosshair_t", "0", "Removes the top line from the crosshair :"
                                               "0 : normal crosshair,"
                                               "1 : crosshair without top.\n");
Variable cl_crosshairgap("cl_crosshairgap", "5", 0, "Changes the distance of the crosshair lines from the center of screen.\n");
Variable cl_crosshaircolor_r("cl_crosshaircolor_r", "0", 0, 255, "Changes the color of the crosshair.\n");
Variable cl_crosshaircolor_g("cl_crosshaircolor_g", "255", 0, 255, "Changes the color of the crosshair.\n");
Variable cl_crosshaircolor_b("cl_crosshaircolor_b", "0", 0, 255, "Changes the color of the crosshair.\n");
Variable cl_crosshairsize("cl_crosshairsize", "5", 0, "Changes the size of the crosshair.\n");
Variable cl_crosshairthickness("cl_crosshairthickness", "0", 0, "Changes the thinkness of the crosshair lines\n.");
Variable cl_crosshairusealpha("cl_crosshairusealpha", "0", "Allows to use transparency.\n");
Variable cl_crosshairalpha("cl_crosshairalpha", "255", 0, 255, "Change the amount of transparency.\n");
Variable cl_crosshairdot("cl_crosshairdot", "1", "Decides if there is a dot in the middle of the crosshair.\n");

Crosshair* crosshair;

bool Crosshair::GetCurrentSize(int& xSize, int& ySize)
{
    return false;
}

void Crosshair::Draw()
{
    if (!sar_crosshair.GetBool()) {
        return;
    }

    Color c(cl_crosshaircolor_r.GetInt(), cl_crosshaircolor_g.GetInt(), cl_crosshaircolor_b.GetInt(), (cl_crosshairusealpha.GetBool()) ? cl_crosshairalpha.GetInt() : 255);

    surface->StartDrawing(surface->matsurface->ThisPtr());

    int xScreen, yScreen, xCenter, yCenter;
    engine->GetScreenSize(xScreen, yScreen);
    xCenter = xScreen / 2;
    yCenter = yScreen / 2;

	int gap = cl_crosshairgap.GetInt();
    int size = cl_crosshairsize.GetInt();
    int thickness = cl_crosshairthickness.GetInt();

    int x1_start, x1_end;
    x1_start = xCenter - gap;
    x1_end = x1_start - size;
    surface->DrawRect(c, x1_end, yCenter-thickness, x1_start, yCenter+thickness+1);

    int x2_start, x2_end;
    x2_start = xCenter + gap;
    x2_end = x2_start + size;
    surface->DrawRect(c, x2_start + 1, yCenter - thickness, x2_end + 1, yCenter + thickness + 1); //Right

    if (!cl_crosshair_t.GetBool()) {
        int y1_start, y1_end;
        y1_start = yCenter - gap;
        y1_end = y1_start - size;
        surface->DrawRect(c, xCenter - thickness, y1_end, xCenter + thickness + 1, y1_start); //Top
    }

    int y2_start, y2_end;
    y2_start = yCenter + gap;
    y2_end = y2_start + size;
    surface->DrawRect(c, xCenter - thickness, y2_start + 1, xCenter + thickness + 1, y2_end + 1); //Bottom

    if (cl_crosshairdot.GetBool()) {
        surface->DrawRect(c, xCenter, yCenter, xCenter + 1, yCenter + 1);
    }
    
	surface->FinishDrawing();
}
