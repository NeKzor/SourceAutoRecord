#include "Module.hpp"

#include <stdexcept>
#include <vector>

Module::Module(const char* filename)
    : filename(filename)
{
}
bool Module::Load()
{
    if (!this->isLoaded) {
        try {
            this->Init();
            this->isLoaded = true;
        } catch (std::exception& ex) {
            throw std::runtime_error(std::string("Module::Init error in ") + this->filename + " -> " + ex.what());
        }
    }
    return this->isLoaded;
}
bool Module::Unload()
{
    if (this->isLoaded) {
        this->Shutdown();
        this->isLoaded = false;
    }
    return this->isLoaded;
}

Modules::Modules()
    : list()
{
}
void Modules::LoadAll()
{
    for (const auto& mod : this->list) {
        mod->Load();
    }
}
void Modules::UnloadAll()
{
    for (const auto& mod : this->list) {
        mod->Unload();
    }
}
Modules::~Modules()
{
    this->UnloadAll();
    for (auto& mod : this->list) {
        if (mod) {
            delete mod;
        }
    }
    this->list.clear();
}
