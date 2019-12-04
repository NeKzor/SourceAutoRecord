#pragma once
#include "Portal2.hpp"

class Portal2Sixense : public Portal2 {
public:
    Portal2Sixense();
    void LoadOffsets() override;
    const char* Version() override;

    static const char* ModDir();
};
