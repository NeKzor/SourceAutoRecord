#include "BlackMesa.hpp"

#include "Game.hpp"
#include "Offsets.hpp"

BlackMesa::BlackMesa()
{
    this->version = SourceGame_BlackMesa;
}
void BlackMesa::LoadOffsets()
{
    using namespace Offsets;

    // engine.dll

    Dtor = 7; // ConVar
    InternalSetValue = 13; // ConVar
    InternalSetFloatValue = 14; // ConVar
    InternalSetIntValue = 15; // ConVar
    ConVarCtor = 492; // setinfo
    Create = 133; // ConVar::ConVar
    GetScreenSize = 5; // CEngineClient
    ClientCmd = 7; // CEngineClient
    Cbuf_AddText = 58; // CEngineClient::ClientCmd
    s_CommandBuffer = 78; // Cbuf_AddText
    AddText = 69; // Cbuf_AddText
    GetLocalPlayer = 12; // CEngineClient
    GetViewAngles = 19; // CEngineClient
    SetViewAngles = 20; // CEngineClient
    GetMaxClients = 21; // CEngineClient
    GetGameDirectory = 35; // CEngineClient
    ServerCmdKeyValues = 129; // CEngineClient
    cl = 4; // CEngineClient::ServerCmdKeyValues
    StringToButtonCode = 32; // CInputSystem
    GetRecordingTick = 1; // CDemoRecorder
    net_time = 14; // CDemoRecorder::GetRecordingTick
    SetSignonState = 3; // CDemoRecorder
    m_bRecording = 1610; // CDemoRecorder::SetSignonState
    m_szDemoBaseName = 1348; // CDemoRecorder::StartupDemoFile
    m_nDemoNumber = 1612; // CDemoRecorder::StartupDemoFile
    StopRecording = 7; // CDemoRecorder
    GetPlaybackTick = 3; // CDemoPlayer
    StartPlayback = 5; // CDemoPlayer
    IsPlayingBack = 6; // CDemoPlayer
    m_szFileName = 4; // CDemoPlayer::SkipToTick
    Paint = 13; // CEngineVGui
    ProcessTick = 1; // CClientState/IServerMessageHandler
    tickcount = 87; // CClientState::ProcessTick
    interval_per_tick = 65; // CClientState::ProcessTick
    Disconnect = 14; //  CClientState
    HostState_OnClientConnected = 613; // CClientState::SetSignonState
    hoststate = 1; // HostState_OnClientConnected
    demoplayer = 110; // CClientState::Disconnect
    demorecorder = 121; // CClientState::Disconnect
    GetCurrentMap = 23; // CEngineTool
    m_szLevelName = 32; // CEngineTool::GetCurrentMap
    AutoCompletionFunc = 62; // listdemo_CompletionFunc
    Key_SetBinding = 113; // unbind
    IsRunningSimulation = 12; // CEngineAPI
    eng = 2; // CEngineAPI::IsRunningSimulation
    Frame = 5; // CEngine
    m_bLoadGame = 407; // CGameClient::ActivatePlayer/CBaseServer::m_szLevelName
    ScreenPosition = 11; // CIVDebugOverlay
    MAX_SPLITSCREEN_PLAYERS = 1; // maxplayers

    // vstdlib.dll

    RegisterConCommand = 9; // CCVar
    UnregisterConCommand = 10; // CCvar
    FindCommandBase = 13; // CCvar
    m_pConCommandList = 44; // CCvar
    IsCommand = 1; // ConCommandBase

    // vgui2.dll

    GetIScheme = 8; // CSchemeManager
    GetFont = 3; // CScheme

    // server.dll

    PlayerMove = 17; // CGameMovement
    CheckJumpButton = 30; // CGameMovement
    FullTossMove = 31; // CGameMovement
    mv = 8; // CGameMovement::CheckJumpButton
    player = 4; // CGameMovement::PlayerMove
    GameFrame = 5; // CServerGameDLL
    GetAllServerClasses = 11; // CServerGameDLL
    IsRestoring = 25; // CServerGameDLL
    Think = 31; // CServerGameDLL
    UTIL_PlayerByIndex = 43; // CServerGameDLL::Think
    gpGlobals = 11; // UTIL_PlayerByIndex
    m_MoveType = 318; // CBasePlayer::UpdateStepSound
    m_iClassName = 92; // CBaseEntity
    m_iName = 268; // CBaseEntity
    S_m_vecAbsOrigin = 640; // CBaseEntity
    S_m_angAbsRotation = 764; // CBaseEntity
    m_iEFlags = 260; // CBaseEntity
    m_flGravity = 612; // CBaseEntity
    NUM_ENT_ENTRIES = 8192; // CBaseEntityList::CBaseEntityList
    GetIServerEntity = 1; // CServerTools
    m_EntPtrArray = 51; // CServerTools::GetIServerEntity

    // client.dll

    GetAllClasses = 10; // CHLClient
    HudProcessInput = 12; // CHLClient
    GetClientMode = 5; // CHLClient::HudProcessInput
    HudUpdate = 13; // CHLClient
    IN_ActivateMouse = 17; // CHLClient
    g_Input = 2; // CHLClient::IN_ActivateMouse
    C_m_vecAbsOrigin = 612; // C_BasePlayer::GetAbsOrigin
    C_m_angAbsRotation = 624; // C_BasePlayer::GetAbsAngles
    GetClientEntity = 3; // CClientEntityList
    CreateMove = 21; // ClientModeShared
    DecodeUserCmdFromBuffer = 7; // CInput
    m_pCommands = 244; // CInput::DecodeUserCmdFromBuffer
    CUserCmdSize = 84; // CInput::DecodeUserCmdFromBuffer
    MULTIPLAYER_BACKUP = 90; // CInput::DecodeUserCmdFromBuffer

    // vguimatsurface.dll

    DrawSetColor = 14; // CMatSystemSurface
    DrawFilledRect = 15; // CMatSystemSurface
    DrawLine = 18; // CMatSystemSurface
    DrawSetTextFont = 20; // CMatSystemSurface
    DrawSetTextColor = 21; // CMatSystemSurface
    GetFontTall = 73; // CMatSystemSurface
    PaintTraverseEx = 117; // CMatSystemSurface
    StartDrawing = 134; // CMatSystemSurface::PaintTraverseEx
    FinishDrawing = 610; // CMatSystemSurface::PaintTraverseEx
    DrawColoredText = 165; // CMatSystemSurface
    DrawTextLen = 168; // CMatSystemSurface
}
const char* BlackMesa::Version()
{
    return "Black Mesa (100001)";
}
const float BlackMesa::Tickrate()
{
    return 64;
}
const char* BlackMesa::ModDir()
{
    return "bms";
}
