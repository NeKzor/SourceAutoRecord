#include "Feature.hpp"

Features::Features()
    : list()
{
}
Features::~Features()
{
    this->DeleteAll();
}
void Features::DeleteAll()
{
    for (auto& feature : this->list) {
        if (feature) {
            delete feature;
        }
    }
    this->list.clear();
}
