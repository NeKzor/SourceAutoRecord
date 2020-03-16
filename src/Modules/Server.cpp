#include "Server.hpp"

#include <cstdint>
#include <cstring>

#include "Features/FovChanger.hpp"
#include "Features/OffsetFinder.hpp"
#include "Features/Routing/EntityInspector.hpp"
#include "Features/Session.hpp"
#include "Features/Speedrun/SpeedrunTimer.hpp"
#include "Features/Stats/Stats.hpp"
#include "Features/StepCounter.hpp"
#include "Features/Tas/AutoStrafer.hpp"
#include "Features/Tas/TasTools.hpp"
#include "Features/Timer/PauseTimer.hpp"
#include "Features/Timer/Timer.hpp"

#include "Engine.hpp"

#include "Game.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

Variable sv_cheats;
Variable sv_footsteps;
Variable sv_alternateticks;
Variable sv_bonus_challenge;
Variable sv_accelerate;
Variable sv_airaccelerate;
Variable sv_friction;
Variable sv_maxspeed;
Variable sv_stopspeed;
Variable sv_maxvelocity;
Variable sv_gravity;

Variable sar_pause("sar_pause", "0", "Enable pause after a load.\n");
Variable sar_pause_at("sar_pause_at", "0", 0, "Pause at the specified tick.\n");
Variable sar_pause_for("sar_pause_for", "0", 0, "Pause for this amount of ticks.\n");
Variable sar_record_at("sar_record_at", "0", 0, "Start recording a demo at the tick specified. Will use sar_record_at_demo_name.\n");
Variable sar_record_at_demo_name("sar_record_at_demo_name", "chamber", "Name of the demo automatically recorded.\n", 0);
Variable sar_record_at_increment("sar_record_at_increment", "0", "Increment automatically the demo name.\n");

void* Server::GetPlayer(int index)
{
    return this->UTIL_PlayerByIndex(index);
}
bool Server::IsPlayer(void* entity)
{
    return Memory::VMT<bool (*)(void*)>(entity, Offsets::IsPlayer)(entity);
}
bool Server::AllowsMovementChanges()
{
    return !sv_bonus_challenge.GetBool() || sv_cheats.GetBool();
}
int Server::GetSplitScreenPlayerSlot(void* entity)
{
    // Simplified version of CBasePlayer::GetSplitScreenPlayerSlot
    for (auto i = 0; i < Offsets::MAX_SPLITSCREEN_PLAYERS; ++i) {
        if (server->UTIL_PlayerByIndex(i + 1) == entity) {
            return i;
        }
    }

    return 0;
}

// CGameMovement::CheckJumpButton
DETOUR_BT(bool, CheckJumpButton)
{
    auto jumped = false;

    if (server->AllowsMovementChanges()) {
        auto mv = *reinterpret_cast<CHLMoveData**>((uintptr_t)thisptr + Offsets::mv);

        if (sar_autojump.GetBool() && !server->jumpedLastTime) {
            mv->m_nOldButtons &= ~IN_JUMP;
        }

        server->jumpedLastTime = false;
        server->savedVerticalVelocity = mv->m_vecVelocity[2];

        server->callFromCheckJumpButton = true;
        jumped = (sar_duckjump.isRegistered && sar_duckjump.GetBool())
            ? CheckJumpButtonBase(thisptr)
            : CheckJumpButton(thisptr);
        server->callFromCheckJumpButton = false;

        if (jumped) {
            server->jumpedLastTime = true;
        }
    } else {
        jumped = CheckJumpButton(thisptr);
    }

    if (jumped) {
        auto player = *reinterpret_cast<void**>((uintptr_t)thisptr + Offsets::player);
        auto stat = stats->Get(server->GetSplitScreenPlayerSlot(player));
        ++stat->jumps->total;
        ++stat->steps->total;
        stat->jumps->StartTrace(server->GetAbsOrigin(player));
    }

    return jumped;
}

// CGameMovement::PlayerMove
DETOUR(PlayerMove)
{
    auto player = *reinterpret_cast<void**>((uintptr_t)thisptr + Offsets::player);
    auto mv = *reinterpret_cast<const CHLMoveData**>((uintptr_t)thisptr + Offsets::mv);

    auto m_fFlags = *reinterpret_cast<int*>((uintptr_t)player + Offsets::m_fFlags);
    auto m_MoveType = *reinterpret_cast<int*>((uintptr_t)player + Offsets::m_MoveType);
    auto m_nWaterLevel = *reinterpret_cast<int*>((uintptr_t)player + Offsets::m_nWaterLevel);

    auto stat = stats->Get(server->GetSplitScreenPlayerSlot(player));

    // Landed after a jump
    if (stat->jumps->isTracing
        && m_fFlags & FL_ONGROUND
        && m_MoveType != MOVETYPE_NOCLIP) {
        stat->jumps->EndTrace(server->GetAbsOrigin(player), sar_stats_jumps_xy.GetBool());
    }

    stepCounter->ReduceTimer(server->gpGlobals->frametime);

    // Player is on ground and moving etc.
    if (stepCounter->stepSoundTime <= 0
        && m_MoveType != MOVETYPE_NOCLIP
        && sv_footsteps.GetFloat()
        && !(m_fFlags & (FL_FROZEN | FL_ATCONTROLS))
        && ((m_fFlags & FL_ONGROUND && mv->m_vecVelocity.Length2D() > 0.0001f) || m_MoveType == MOVETYPE_LADDER)) {
        stepCounter->Increment(m_fFlags, m_MoveType, mv->m_vecVelocity, m_nWaterLevel);
        ++stat->steps->total;
    }

    stat->velocity->Save(server->GetLocalVelocity(player), sar_stats_velocity_peak_xy.GetBool());
    inspector->Record();

    return PlayerMove(thisptr);
}

// CGameMovement::ProcessMovement
DETOUR(ProcessMovement, void* pPlayer, CMoveData* pMove)
{
    if (sv_cheats.GetBool()) {
        autoStrafer->Strafe(pPlayer, pMove);
        tasTools->SetAngles(pPlayer);
    }

    return ProcessMovement(thisptr, pPlayer, pMove);
}

// CGameMovement::FinishGravity
DETOUR(FinishGravity)
{
    if (server->callFromCheckJumpButton) {
        if (sar_duckjump.GetBool()) {
            auto player = *reinterpret_cast<uintptr_t*>((uintptr_t)thisptr + Offsets::player);
            auto mv = *reinterpret_cast<CHLMoveData**>((uintptr_t)thisptr + Offsets::mv);

            auto m_pSurfaceData = *reinterpret_cast<uintptr_t*>(player + Offsets::m_pSurfaceData);
            auto m_bDucked = *reinterpret_cast<bool*>(player + Offsets::m_bDucked);
            auto m_fFlags = *reinterpret_cast<int*>(player + Offsets::m_fFlags);

            auto flGroundFactor = (m_pSurfaceData) ? *reinterpret_cast<float*>(m_pSurfaceData + Offsets::jumpFactor) : 1.0f;
            auto flMul = std::sqrt(2 * sv_gravity.GetFloat() * GAMEMOVEMENT_JUMP_HEIGHT);

            if (m_bDucked || m_fFlags & FL_DUCKING) {
                mv->m_vecVelocity[2] = flGroundFactor * flMul;
            } else {
                mv->m_vecVelocity[2] = server->savedVerticalVelocity + flGroundFactor * flMul;
            }
        }

        if (sar_jumpboost.GetBool()) {
            auto player = *reinterpret_cast<uintptr_t*>((uintptr_t)thisptr + Offsets::player);
            auto mv = *reinterpret_cast<CHLMoveData**>((uintptr_t)thisptr + Offsets::mv);

            auto m_bDucked = *reinterpret_cast<bool*>(player + Offsets::m_bDucked);

            Vector vecForward;
            Math::AngleVectors(mv->m_vecViewAngles, &vecForward);
            vecForward.z = 0;
            Math::VectorNormalize(vecForward);

            float flSpeedBoostPerc = (!mv->m_bIsSprinting && !m_bDucked) ? 0.5f : 0.1f;
            float flSpeedAddition = std::fabs(mv->m_flForwardMove * flSpeedBoostPerc);
            float flMaxSpeed = mv->m_flMaxSpeed + (mv->m_flMaxSpeed * flSpeedBoostPerc);
            float flNewSpeed = flSpeedAddition + mv->m_vecVelocity.Length2D();

            if (sar_jumpboost.GetInt() == 1) {
                if (flNewSpeed > flMaxSpeed) {
                    flSpeedAddition -= flNewSpeed - flMaxSpeed;
                }

                if (mv->m_flForwardMove < 0.0f) {
                    flSpeedAddition *= -1.0f;
                }
            }

            Math::VectorAdd(vecForward * flSpeedAddition, mv->m_vecVelocity, mv->m_vecVelocity);
        }
    }

    return FinishGravity(thisptr);
}

// CGameMovement::AirMove
DETOUR_B(AirMove)
{
    if (sar_aircontrol.GetInt() >= 2 && server->AllowsMovementChanges()) {
        return AirMoveBase(thisptr);
    }

    return AirMove(thisptr);
}

// CServerGameDLL::GameFrame
#ifdef _WIN32
DETOUR_STD(void, GameFrame, bool simulating)
#else
DETOUR(GameFrame, bool simulating)
#endif
{
    if (simulating && sar_record_at.GetFloat() > 0 && sar_record_at.GetFloat() == session->GetTick()) {
        std::string cmd = std::string("record ") + sar_record_at_demo_name.GetString();
        engine->ExecuteCommand(cmd.c_str());
    }

    if (!server->IsRestoring()) {
        if (!simulating && !pauseTimer->IsActive()) {
            pauseTimer->Start();
        } else if (simulating && pauseTimer->IsActive()) {
            pauseTimer->Stop();
            console->DevMsg("Paused for %i non-simulated ticks.\n", pauseTimer->GetTotal());
        }
    }

#ifdef _WIN32
    GameFrame(simulating);
#else
    auto result = GameFrame(thisptr, simulating);
#endif

    if (sar_pause.GetBool()) {
        if (!server->paused && sar_pause_at.GetInt() == session->GetTick() && simulating) {
            engine->ExecuteCommand("pause");
            server->paused = true;
            server->pauseTick = engine->GetTick();
        } else if (server->paused && !simulating) {
            if (sar_pause_for.GetInt() > 0 && sar_pause_for.GetInt() + engine->GetTick() == server->pauseTick) {
                engine->ExecuteCommand("unpause");
                server->paused = false;
            }
            ++server->pauseTick;
        } else if (server->paused && simulating && engine->GetTick() > server->pauseTick + 5) {
            server->paused = false;
        }
    }

    if (session->isRunning && session->GetTick() == 16) {
        fovChanger->Force();
    }

    if (session->isRunning && pauseTimer->IsActive()) {
        pauseTimer->Increment();

        if (speedrun->IsActive() && sar_speedrun_time_pauses.GetBool()) {
            speedrun->IncrementPauseTime();
        }

        if (timer->isRunning && sar_timer_time_pauses.GetBool()) {
            ++timer->totalTicks;
        }
    }

    if (session->isRunning && sar_speedrun_standard.GetBool()) {
        speedrun->CheckRules(engine->GetTick());
    }

#ifndef _WIN32
    return result;
#endif
}

void Server::Init()
{
    this->g_GameMovement = Interface::Hookable(this, "GameMovement0");
    this->g_ServerGameDLL = Interface::Hookable(this, "ServerGameDLL0");

    this->g_GameMovement->Hook(&hkCheckJumpButton, Offsets::CheckJumpButton);
    this->g_GameMovement->Hook(&hkPlayerMove, Offsets::PlayerMove);

    if (sar.game->Is(SourceGame_Portal2Engine)) {
        this->g_GameMovement->Hook(&hkProcessMovement, Offsets::ProcessMovement);
        this->g_GameMovement->Hook(&hkFinishGravity, Offsets::FinishGravity);
        this->g_GameMovement->Hook(&hkAirMove, Offsets::AirMove);

        auto ctor = this->g_GameMovement->Original(0);
        auto baseCtor = Memory::Read(ctor + Offsets::AirMove_Offset1);
        auto baseOffset = Memory::Deref<uintptr_t>(baseCtor + Offsets::AirMove_Offset2);
        Memory::Deref<_AirMove>(baseOffset + Offsets::AirMove * sizeof(uintptr_t), &AirMoveBase);

        Memory::Deref<_CheckJumpButton>(baseOffset + Offsets::CheckJumpButton * sizeof(uintptr_t), &CheckJumpButtonBase);
    }

    Interface::Temp(this, "VSERVERTOOLS0", [this](const Interface* g_ServerTools) {
        auto GetIServerEntity = g_ServerTools->Original(Offsets::GetIServerEntity);
        Memory::Deref(GetIServerEntity + Offsets::m_EntPtrArray, &this->m_EntPtrArray);
    });

    auto Think = this->g_ServerGameDLL->Original(Offsets::Think);
    Memory::Read<_UTIL_PlayerByIndex>(Think + Offsets::UTIL_PlayerByIndex, &this->UTIL_PlayerByIndex);
    Memory::DerefDeref<CGlobalVars*>((uintptr_t)this->UTIL_PlayerByIndex + Offsets::gpGlobals, &this->gpGlobals);

    this->GetAllServerClasses = this->g_ServerGameDLL->Original<_GetAllServerClasses>(Offsets::GetAllServerClasses);
    this->IsRestoring = this->g_ServerGameDLL->Original<_IsRestoring>(Offsets::IsRestoring);

    if (sar.game->Is(SourceGame_Portal2Game | SourceGame_Portal)) {
        this->g_ServerGameDLL->Hook(&hkGameFrame, Offsets::GameFrame);
    }

    offsetFinder->ServerSide("CBasePlayer", "m_nWaterLevel", &Offsets::m_nWaterLevel);
    offsetFinder->ServerSide("CBasePlayer", "m_iName", &Offsets::m_iName);
    offsetFinder->ServerSide("CBasePlayer", "m_vecVelocity[0]", &Offsets::S_m_vecVelocity);
    offsetFinder->ServerSide("CBasePlayer", "m_fFlags", &Offsets::m_fFlags);
    offsetFinder->ServerSide("CBasePlayer", "m_flMaxspeed", &Offsets::m_flMaxspeed);
    offsetFinder->ServerSide("CBasePlayer", "m_vecViewOffset[0]", &Offsets::S_m_vecViewOffset);

    if (sar.game->Is(SourceGame_Portal2Engine)) {
        offsetFinder->ServerSide("CBasePlayer", "m_bDucked", &Offsets::m_bDucked);
        offsetFinder->ServerSide("CBasePlayer", "m_flFriction", &Offsets::m_flFriction);
    }

    if (sar.game->Is(SourceGame_Portal2Game)) {
        offsetFinder->ServerSide("CPortal_Player", "iNumPortalsPlaced", &Offsets::iNumPortalsPlaced);
    }

    sv_cheats = Variable("sv_cheats");
    sv_footsteps = Variable("sv_footsteps");
    sv_alternateticks = Variable("sv_alternateticks");
    sv_bonus_challenge = Variable("sv_bonus_challenge");
    sv_accelerate = Variable("sv_accelerate");
    sv_airaccelerate = Variable("sv_airaccelerate");
    sv_friction = Variable("sv_friction");
    sv_maxspeed = Variable("sv_maxspeed");
    sv_stopspeed = Variable("sv_stopspeed");
    sv_maxvelocity = Variable("sv_maxvelocity");
    sv_gravity = Variable("sv_gravity");
}
void Server::Shutdown()
{
    Interface::Destroy(this->g_GameMovement);
    Interface::Destroy(this->g_ServerGameDLL);
}

Server* server;
