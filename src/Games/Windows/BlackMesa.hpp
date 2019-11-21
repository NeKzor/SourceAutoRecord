#pragma once
#include "Game.hpp"

class BlackMesa : public Game {
public:
    BlackMesa();
    void LoadOffsets() override;
    const char* Version() override;
    const float Tickrate() override;

    static const char* ModDir();
};
