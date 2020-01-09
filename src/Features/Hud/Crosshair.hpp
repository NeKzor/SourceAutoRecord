#pragma once
#include "Variable.hpp"
#include "Hud.hpp"

class Crosshair : public Hud {
public:
    bool GetCurrentSize(int& xSize, int& ySize) override;
	void Draw() override;
};

extern Crosshair* crosshair;

extern Variable sar_crosshair;
extern Variable cl_crosshair_t;
extern Variable cl_crosshairgap;
extern Variable cl_crosshaircolor;
extern Variable cl_crosshairsize;
extern Variable cl_crosshairthickness;
extern Variable cl_crosshairusealpha;
extern Variable cl_crosshairalpha;
extern Variable cl_crosshairdot;