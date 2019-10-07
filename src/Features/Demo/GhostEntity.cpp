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
    , ID()
    , name()
    , currentMap()
    , sameMap(false)
    , ghost_entity(nullptr)
    , startTick()
    , CMTime(0)
    , modelName("models/props/food_can/food_can_open.mdl")
    , isPlaying(false)
    , hasFinished(false)
    , tickCount(0)
    , startDelay(0)
    , demo()
    , newPos({{ 1, 1, 1 }, { 1, 1, 1 }})
    , oldPos({ { 1, 1, 1 }, { 1, 1, 1 } })
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

GhostEntity* GhostEntity::Spawn(bool instantPlay, bool playerPos, Vector position)
{
    this->ghost_entity = server->CreateEntityByName("prop_dynamic_override");
    server->SetKeyValueChar(this->ghost_entity, "model", this->modelName);
    server->SetKeyValueChar(this->ghost_entity, "targetname", "ghost");

    Vector pos;
    if (!playerPos && (position.x == 0 || position.y == 0 || position.z == 0)) {
        pos = this->positionList[(this->tickCount)];
    } else if (!playerPos && (position.x != 0 || position.y != 0 || position.z != 0)) {
        pos = position;
    } else if (playerPos) {
        pos = server->GetAbsOrigin(server->GetPlayer(GET_SLOT() + 1));
        pos.z += sar_ghost_height.GetFloat();
    }

    this->SetPosAng(pos, Vector{ 0, 0, 0 });

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
        this->lastUpdate = this->clock.now();
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
    if (this->ghost_entity == nullptr && !this->hasFinished && ((engine->GetMaxClients() == 1 && tick >= (this->startTick + (this->CMTime - this->demo.playbackTicks))) || (engine->GetMaxClients() > 1 && tick >= this->startTick))) {
        this->Spawn();
    }

    if (this->isPlaying) {
        Vector position = this->positionList[(this->tickCount)];
        position.z += sar_ghost_height.GetFloat();
        this->SetPosAng(position, this->angleList[(this->tickCount)]);

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
        this->hasFinished = true;
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

void GhostEntity::ChangeModel(std::string modelName)
{
    std::copy(modelName.begin(), modelName.end(), this->modelName);
    this->modelName[sizeof(this->modelName)] = '\0';
}

void GhostEntity::SetPosAng(const Vector& pos, const Vector& ang)
{
    server->SetKeyValueVector(this->ghost_entity, "origin", pos);
    server->SetKeyValueVector(this->ghost_entity, "angles", ang);
    this->currentPos = pos;
}

void GhostEntity::Lerp(DataGhost& oldPosition, DataGhost& targetPosition, float time)
{
    Vector newPos;
    newPos.x = (1 - time) * oldPosition.position.x + time * targetPosition.position.x;
    newPos.y = (1 - time) * oldPosition.position.y + time * targetPosition.position.y;
    newPos.z = (1 - time) * oldPosition.position.z + time * targetPosition.position.z;

    Vector newAngle;
    newAngle.x = (1 - time) * oldPosition.view_angle.x + time * targetPosition.view_angle.x;
    newAngle.y = (1 - time) * oldPosition.view_angle.y + time * targetPosition.view_angle.y;
    newAngle.z = 0;

	this->SetPosAng(newPos, newAngle);
}
