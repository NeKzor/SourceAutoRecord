#pragma once
#include "HalfLife2.hpp"

class Ghosting : public HalfLife2 {
public:
    Ghosting();
    void LoadOffsets() override;
    const char* Version() override;
    const float Tickrate() override;

    static const char* ModDir();
};
