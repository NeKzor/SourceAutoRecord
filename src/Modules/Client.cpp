#include "Client.hpp"

#include <cstdint>
#include <cstring>

#include "Features/Hud/InputHud.hpp"
#include "Features/Imitator.hpp"
#include "Features/OffsetFinder.hpp"
#include "Features/ReplaySystem/ReplayPlayer.hpp"
#include "Features/ReplaySystem/ReplayProvider.hpp"
#include "Features/ReplaySystem/ReplayRecorder.hpp"
#include "Features/Session.hpp"
#include "Features/Tas/AutoStrafer.hpp"
#include "Features/Tas/CommandQueuer.hpp"

#include "Console.hpp"
#include "Engine.hpp"
#include "Server.hpp"

#include "Command.hpp"
#include "Game.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"

Variable cl_showpos;
Variable cl_sidespeed;
Variable cl_forwardspeed;
Variable in_forceuser;
Variable cl_fov;

void* Client::GetPlayer(int index)
{
    return this->GetClientEntity(this->s_EntityList->ThisPtr(), index);
}
void Client::CalcButtonBits(int nSlot, int& bits, int in_button, int in_ignore, kbutton_t* button, bool reset)
{
    auto pButtonState = &button->GetPerUser(nSlot);
    if (pButtonState->state & 3) {
        bits |= in_button;
    }

    int clearmask = ~2;
    if (in_ignore & in_button) {
        clearmask = ~3;
    }

    if (reset) {
        pButtonState->state &= clearmask;
    }
}

// CHLClient::HudUpdate
DETOUR(HudUpdate, unsigned int a2)
{
    if (cmdQueuer->isRunning) {
        for (auto&& tas = cmdQueuer->frames.begin(); tas != cmdQueuer->frames.end();) {
            --tas->framesLeft;

            if (tas->framesLeft <= 0) {
                console->DevMsg("[%i] %s\n", session->currentFrame, tas->command.c_str());

                if (sar.game->Is(SourceGame_Portal2Engine)) {
                    if (engine->GetMaxClients() <= 1) {
                        engine->Cbuf_AddText(tas->splitScreen, tas->command.c_str(), 0);
                    } else {
                        auto entity = engine->PEntityOfEntIndex(tas->splitScreen + 1);
                        if (entity && !entity->IsFree() && server->IsPlayer(entity->m_pUnk)) {
                            engine->ClientCommand(nullptr, entity, tas->command.c_str());
                        }
                    }
                } else if (sar.game->Is(SourceGame_HalfLife2Engine)) {
                    engine->AddText(engine->s_CommandBuffer, tas->command.c_str(), 0);
                }

                tas = cmdQueuer->frames.erase(tas);
            } else {
                ++tas;
            }
        }
    }

    ++session->currentFrame;
    return HudUpdate(thisptr, a2);
}

// ClientModeShared::CreateMove
DETOUR(CreateMove, float flInputSampleTime, CUserCmd* cmd)
{
    if (cmd->command_number) {
        if (replayPlayer1->IsPlaying()) {
            replayPlayer1->Play(replayProvider->GetCurrentReplay(), cmd);
        } else if (replayRecorder1->IsRecording()) {
            replayRecorder1->Record(replayProvider->GetCurrentReplay(), cmd);
        }
    }

    if (sar_mimic.isRegistered && sar_mimic.GetBool()) {
        imitator->Save(cmd);
    }

    if (!in_forceuser.isReference || (in_forceuser.isReference && !in_forceuser.GetBool())) {
        inputHud.SetButtonBits(0, cmd->buttons);
    }

    return CreateMove(thisptr, flInputSampleTime, cmd);
}
DETOUR(CreateMove2, float flInputSampleTime, CUserCmd* cmd)
{
    if (cmd->command_number) {
        if (replayPlayer2->IsPlaying()) {
            replayPlayer2->Play(replayProvider->GetCurrentReplay(), cmd);
        } else if (replayRecorder2->IsRecording()) {
            replayRecorder2->Record(replayProvider->GetCurrentReplay(), cmd);
        }
    }

    if (sar_mimic.GetBool() && (!sv_bonus_challenge.GetBool() || sv_cheats.GetBool())) {
        imitator->Modify(cmd);
    }

    if (in_forceuser.GetBool()) {
        inputHud.SetButtonBits(1, cmd->buttons);
    }

    return CreateMove2(thisptr, flInputSampleTime, cmd);
}

// CHud::GetName
DETOUR_T(const char*, GetName)
{
    // Never allow CHud::FindElement to find this HUD
    if (sar_disable_challenge_stats_hud.GetBool())
        return "";

    return GetName(thisptr);
}

// CInput::DecodeUserCmdFromBuffer
DETOUR(DecodeUserCmdFromBuffer, int nSlot, int buf, signed int sequence_number)
{
    auto result = DecodeUserCmdFromBuffer(thisptr, nSlot, buf, sequence_number);

    auto m_pCommands = *reinterpret_cast<uintptr_t*>((uintptr_t)thisptr + nSlot * Offsets::PerUserInput_tSize + Offsets::m_pCommands);
    auto cmd = reinterpret_cast<CUserCmd*>(m_pCommands + Offsets::CUserCmdSize * (sequence_number % Offsets::MULTIPLAYER_BACKUP));

    inputHud.SetButtonBits(0, cmd->buttons);

    return result;
}
DETOUR(DecodeUserCmdFromBuffer2, int buf, signed int sequence_number)
{
    auto result = DecodeUserCmdFromBuffer2(thisptr, buf, sequence_number);

    auto m_pCommands = *reinterpret_cast<uintptr_t*>((uintptr_t)thisptr + Offsets::m_pCommands);
    auto cmd = reinterpret_cast<CUserCmd*>(m_pCommands + Offsets::CUserCmdSize * (sequence_number % Offsets::MULTIPLAYER_BACKUP));

    inputHud.SetButtonBits(1, cmd->buttons);

    return result;
}

// CInput::CreateMove
DETOUR(CInput_CreateMove, int sequence_number, float input_sample_frametime, bool active)
{
    auto originalValue = 0;
    if (sar_tas_ss_forceuser.GetBool()) {
        originalValue = in_forceuser.GetInt();
        in_forceuser.SetValue(GET_SLOT());
    }

    auto result = CInput_CreateMove(thisptr, sequence_number, input_sample_frametime, active);

    if (sar_tas_ss_forceuser.GetBool()) {
        in_forceuser.SetValue(originalValue);
    }

    return result;
}

// CInput::GetButtonBits
DETOUR(GetButtonBits, bool bResetState)
{
    auto bits = GetButtonBits(thisptr, bResetState);

    client->CalcButtonBits(GET_SLOT(), bits, IN_AUTOSTRAFE, 0, &autoStrafer->in_autostrafe, bResetState);

    return bits;
}

DETOUR_COMMAND(playvideo_end_level_transition)
{
    console->DevMsg("%s\n", args.m_pArgSBuffer);
    session->Ended();

    return playvideo_end_level_transition_callback(args);
}

void Client::Init()
{
    bool readJmp = false;
#ifdef _WIN32
    readJmp = sar.game->Is(SourceGame_TheStanleyParable | SourceGame_TheBeginnersGuide);
#endif

    this->g_ClientDLL = Interface::Hookable(this, "VClient0");
    this->GetAllClasses = this->g_ClientDLL->Original<_GetAllClasses>(Offsets::GetAllClasses, readJmp);

    this->g_ClientDLL->Hook(&hkHudUpdate, Offsets::HudUpdate);

    if (sar.game->Is(SourceGame_Portal2)) {
        auto leaderboard = Command("+leaderboard");
        if (!!leaderboard) {
            using _GetHud = void*(__cdecl*)(int unk);
            using _FindElement = void*(__rescall*)(void* thisptr, const char* pName);

            auto cc_leaderboard_enable = (uintptr_t)leaderboard.ThisPtr()->m_pCommandCallback;
            auto GetHud = Memory::Read<_GetHud>(cc_leaderboard_enable + Offsets::GetHud);
            auto FindElement = Memory::Read<_FindElement>(cc_leaderboard_enable + Offsets::FindElement);
            auto CHUDChallengeStats = FindElement(GetHud(-1), "CHUDChallengeStats");

            if (this->g_HUDChallengeStats = Interface::Hookable(this, CHUDChallengeStats)) {
                this->g_HUDChallengeStats->Hook(&hkGetName, Offsets::GetName);
            }
        }
    }

    auto IN_ActivateMouse = this->g_ClientDLL->Original(Offsets::IN_ActivateMouse, readJmp);
    auto g_InputAddr = Memory::DerefDeref<uintptr_t>(IN_ActivateMouse + Offsets::g_Input);

    this->g_Input = Interface::Hookable(this, g_InputAddr);
    if (sar.game->Is(SourceGame_Portal2Engine)) {
        this->g_Input->Hook(&hkDecodeUserCmdFromBuffer, Offsets::DecodeUserCmdFromBuffer);
        this->g_Input->Hook(&hkGetButtonBits, Offsets::GetButtonBits);

        auto JoyStickApplyMovement = this->g_Input->Original(Offsets::JoyStickApplyMovement, readJmp);
        Memory::Read(JoyStickApplyMovement + Offsets::KeyDown, &this->KeyDown);
        Memory::Read(JoyStickApplyMovement + Offsets::KeyUp, &this->KeyUp);

        if (sar.game->Is(SourceGame_TheStanleyParable)) {
            auto GetButtonBits = this->g_Input->Original(Offsets::GetButtonBits, readJmp);
            Memory::Deref(GetButtonBits + Offsets::in_jump, &this->in_jump);
        } else if (sar.game->Is(SourceGame_Portal2 | SourceGame_ApertureTag)) {
            in_forceuser = Variable("in_forceuser");
            this->g_Input->Hook(&hkCInput_CreateMove, Offsets::GetButtonBits + 1);
            playvideo_end_level_transition_hook.Register(this);
        }
    } else {
        this->g_Input->Hook(&hkDecodeUserCmdFromBuffer2, Offsets::DecodeUserCmdFromBuffer);
    }

    auto HudProcessInput = this->g_ClientDLL->Original(Offsets::HudProcessInput, readJmp);
    uintptr_t clientMode = 0;
    uintptr_t clientMode2 = 0;

    if (sar.game->Is(SourceGame_Portal2Engine)) {
        if (sar.game->Is(SourceGame_Portal2 | SourceGame_ApertureTag)) {
            auto GetClientMode = Memory::Read<uintptr_t>(HudProcessInput + Offsets::GetClientMode);
            auto g_pClientMode = Memory::Deref<uintptr_t>(GetClientMode + Offsets::g_pClientMode);
            clientMode = Memory::Deref<uintptr_t>(g_pClientMode);
            clientMode2 = Memory::Deref<uintptr_t>(g_pClientMode + sizeof(uintptr_t));
        } else {
            typedef uintptr_t (*_GetClientMode)();
            auto GetClientMode = Memory::Read<_GetClientMode>(HudProcessInput + Offsets::GetClientMode);
            clientMode = GetClientMode();
        }
    } else if (sar.game->Is(SourceGame_HalfLife2Engine)) {
        clientMode = Memory::DerefDeref<uintptr_t>(HudProcessInput + Offsets::GetClientMode);
    }

    this->g_pClientMode = Interface::Hookable(this, clientMode);
    this->g_pClientMode->Hook(&hkCreateMove, Offsets::CreateMove);

    this->g_pClientMode2 = Interface::Hookable(this, clientMode2);
    this->g_pClientMode2->Hook(&hkCreateMove2, Offsets::CreateMove);

    this->s_EntityList = Interface::CreateNew(this, "VClientEntityList0");
    this->GetClientEntity = this->s_EntityList->Original<_GetClientEntity>(Offsets::GetClientEntity, readJmp);

    offsetFinder->ClientSide("CBasePlayer", "m_vecVelocity[0]", &Offsets::C_m_vecVelocity);
    offsetFinder->ClientSide("CBasePlayer", "m_vecViewOffset[0]", &Offsets::C_m_vecViewOffset);

    cl_showpos = Variable("cl_showpos");
    cl_sidespeed = Variable("cl_sidespeed");
    cl_forwardspeed = Variable("cl_forwardspeed");
    cl_fov = Variable("cl_fov");
}
void Client::Shutdown()
{
    Interface::Destroy(this->g_ClientDLL);
    Interface::Destroy(this->g_pClientMode);
    Interface::Destroy(this->g_pClientMode2);
    Interface::Destroy(this->g_HUDChallengeStats);
    Interface::Destroy(this->s_EntityList);
    Interface::Destroy(this->g_Input);
}

Client* client;
