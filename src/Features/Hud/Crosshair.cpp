#include "Crosshair.hpp"

#include "Variable.hpp"

#include "Modules/Surface.hpp"
#include "Modules/Engine.hpp"

Variable sar_crosshair("sar_crosshair", "0", "Enable custom crosshair.\n");
Variable cl_crosshair_t("cl_crosshair_t", "1", "Removes the top line from the crosshair :"
                                               "0 : normal crosshair,"
                                               "1 : crosshair without top.\n");
Variable cl_crosshairgap("cl_crosshairgap", "5", 0, "Changes the distance of the crosshair lines from the center of screen.\n");
Variable cl_crosshaircolor_r("cl_croshaircolor_r", "0", 0, 255, "Changes the color of the crosshair.\n");
Variable cl_crosshaircolor_g("cl_croshaircolor_g", "0", 0, 255, "Changes the color of the crosshair.\n");
Variable cl_crosshaircolor_b("cl_croshaircolor_b", "255", 0, 255, "Changes the color of the crosshair.\n");
Variable cl_crosshairsize("cl_crosshairsize", "5", 0, "Changes the size of the crosshair.\n");
Variable cl_crosshairthickness("cl_crosshairthickness", "1", 1, "Changes the thinkness of the crosshair lines\n.");
Variable cl_crosshairusealpha("cl_crosshairusealpha", "0", "Allows to use transparency.\n");
Variable cl_crosshairalpha("cl_crosshairalpha", "0", 0, 255, "Change the amount of transparency.\n");
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

    surface->StartDrawing(surface->matsurface->ThisPtr());

    int xScreen, yScreen, xCenter, yCenter;
    engine->GetScreenSize(xScreen, yScreen);
    xCenter = xScreen / 2;
    yCenter = yScreen / 2;

    int x1_start, x1_end;
    int y1_start, y1_end;
    int x2_start, x2_end;
    int y2_start, y2_end;

    x1_start = xCenter - cl_crosshairgap.GetInt();
    x1_end = x1_start - cl_crosshairsize.GetInt();

    x2_start = xCenter + cl_crosshairgap.GetInt();
    x2_end = x2_start + cl_crosshairsize.GetInt();

    y1_start = yCenter - cl_crosshairgap.GetInt();
    y1_end = y1_start - cl_crosshairsize.GetInt();

    y2_start = yCenter + cl_crosshairgap.GetInt();
    y2_end = y2_start + cl_crosshairsize.GetInt();


	Color c(cl_crosshaircolor_r.GetInt(), cl_crosshaircolor_g.GetInt(), cl_crosshaircolor_b.GetInt());
    surface->Drawline(c, x1_end, yCenter, x1_start, yCenter); //Left
    surface->Drawline(c, x2_start, yCenter, x2_end, yCenter); //Right
    surface->Drawline(c, xCenter, y1_end, xCenter, y1_start); //Top
    surface->Drawline(c, xCenter, y2_start, xCenter, y2_end); //Bottom

	/*surface->DrawRect(c, x1_end, yCenter, x1_start, yCenter + 1); //Left
	surface->DrawRect(c, x2_start, yCenter, x2_end, yCenter+1); //Right
    surface->DrawRect(c, xCenter, y1_end, xCenter + 1, y1_start); //Top
    surface->DrawRect(c, xCenter, y2_start, xCenter+1, y2_end); //Bottom*/
    
	//surface->FinishDrawing();
}