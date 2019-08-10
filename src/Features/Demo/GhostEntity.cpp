#include "Features/Demo/GhostEntity.hpp"
#include "Features/Demo/GhostPlayer.hpp"

#include "Features/Demo/Demo.hpp"
#include "Features/Demo/DemoParser.hpp"
#include "Features/Session.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"

GhostEntity::GhostEntity()
    : positionList()
    , angleList()
    , ghost_entity(nullptr)
    , startTick()
    , CMTime(0)
    , modelName("models/props/food_can/food_can_open.mdl")
    , isPlaying(false)
    , tickCount(0)
    , startDelay(0)
    , demo()
    , trail(nullptr)
{
}

void GhostEntity::Reset()
{
    this->ghost_entity = nullptr;
    this->isPlaying = false;
    this->tickCount = GetStartDelay();
}

void GhostEntity::Stop()
{
    this->Reset();
    delete this->ghost_entity;
}

GhostEntity* GhostEntity::Spawn(bool instantPlay, bool playerPos)
{
    this->ghost_entity = server->CreateEntityByName("prop_dynamic_override");
    server->SetKeyValueChar(this->ghost_entity, "model", this->modelName);
    server->SetKeyValueChar(this->ghost_entity, "targetname", "ghost");
    if (!playerPos) {
        server->SetKeyValueVector(this->ghost_entity, "origin", this->positionList[(this->tickCount)]);
    } else {
        Vector pos = server->GetAbsOrigin(server->GetPlayer(GET_SLOT() + 1));
        pos.z += sar_ghost_height.GetFloat();
        server->SetKeyValueVector(this->ghost_entity, "origin", pos);
    }

    server->SetKeyValueChar(this->ghost_entity, "angles", "0 0 0");
    if (sar_ghost_transparency.GetFloat() <= 254) {
        server->SetKeyValueChar(this->ghost_entity, "rendermode", "1");
        server->SetKeyValueFloat(this->ghost_entity, "renderamt", sar_ghost_transparency.GetFloat());
    } else {
        server->SetKeyValueChar(this->ghost_entity, "rendermode", "0");
    }

    server->DispatchSpawn(this->ghost_entity);
    this->isPlaying = instantPlay;

    if (this->ghost_entity != nullptr) {
        console->Print("Say 'Hi !' to Jonnil !\n");
        return this;
    }

    return nullptr;
}

bool GhostEntity::IsReady()
{
    if (ghostPlayer->enabled && this->CMTime > 0) { //No need to check positionList anymore, cause CMTime is > 0 if PositionList > 0
        return true;
    }
    return false;
}

void GhostEntity::SetCMTime(float playbackTime)
{
    float time = (playbackTime)*60;
    this->CMTime = std::round(time);
}

void GhostEntity::Think()
{
    auto tick = session->GetTick();
    if ((engine->GetMaxClients() == 1 && tick == (this->startTick + (this->CMTime - this->demo.playbackTicks))) || (engine->GetMaxClients() > 1 && tick == this->startTick)) {
        this->Spawn();
        if (sar_ghost_trail_lenght.GetFloat() > 0) {
            this->CreateTrail();
        }
    }

    if (this->isPlaying) {
        Vector position = this->positionList[(this->tickCount)];
        position.z += sar_ghost_height.GetFloat();
        server->SetKeyValueVector(this->ghost_entity, "origin", position);
        server->SetKeyValueVector(this->ghost_entity, "angles", this->angleList[(this->tickCount)]);

        if (engine->GetMaxClients() == 1) {
            if (tick % 2 == 0) {
                this->tickCount++;
            }
        } else {
            this->tickCount++;
        }
    }
    if (this->tickCount == this->positionList.size()) {
        console->Print("Ghost has finished.\n");
        this->Reset();
    }
}

int GhostEntity::GetStartDelay()
{
    return this->startDelay;
}

void GhostEntity::SetStartDelay(int delay)
{
    this->startDelay = delay;
}

void GhostEntity::ChangeModel(const char modelName[64])
{
    std::strncpy(this->modelName, modelName, sizeof(this->modelName));
}

void GhostEntity::CreateTrail()
{
    this->trail = server->CreateEntityByName("env_spritetrail");
    server->SetKeyValueChar(this->trail, "spritename", "materials/sprites/laserbeam.vmt");

    Vector pos = this->positionList[(this->tickCount)];
    pos.z += sar_ghost_height.GetFloat();
    server->SetKeyValueVector(this->trail, "origin", pos);

    server->SetKeyValueChar(this->trail, "targetname", "trail");
    server->SetKeyValueChar(this->trail, "parentname", "ghost");
    server->SetKeyValueChar(this->trail, "rendermode", "5");
    server->SetKeyValueFloat(this->trail, "lifetime", sar_ghost_trail_lenght.GetFloat());
    server->SetKeyValueFloat(this->trail, "renderamt", sar_ghost_trail_transparency.GetFloat());
    server->SetKeyValueChar(this->trail, "startwidth", "9.5");
    server->SetKeyValueChar(this->trail, "endwidth", "1.05");
    server->DispatchSpawn(trail);
    engine->ExecuteCommand("ent_fire trail setparent ghost");
}
