#include "Portal2Sixense.hpp"

#include "Game.hpp"
#include "Offsets.hpp"

Portal2Sixense::Portal2Sixense()
{
    this->version = SourceGame_Portal2Sixense;
}
void Portal2Sixense::LoadOffsets()
{
    Portal2::LoadOffsets();

    using namespace Offsets;

    // engine.dll

    interval_per_tick = 73; // CClientState::ProcessTick
    tickcount = 103; // CClientState::ProcessTick
    HostState_OnClientConnected = 695; // CClientState::SetSignonState

    // client.dll

    JoyStickApplyMovement = 61; // CInput
    KeyUp = 234; // CInput::JoyStickApplyMovement
    KeyDown = 255; // CInput::JoyStickApplyMovement
    PerUserInput_tSize = 212; // CInput::DecodeUserCmdFromBuffer
    m_pCommands = 224; // CInput::DecodeUserCmdFromBuffer
    CUserCmdSize = 156; // CInput::DecodeUserCmdFromBuffer

    // vguimatsurface.dll

    DrawColoredText = 159; // CMatSystemSurface
    DrawTextLen = 162; // CMatSystemSurface
}
const char* Portal2Sixense::Version()
{
    return "Portal 2 Sixense Perceptual Pack (5080)";
}
const char* Portal2Sixense::ModDir()
{
    return "portal2_sixense";
}
