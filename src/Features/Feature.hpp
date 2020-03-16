#pragma once
#include <vector>

class Feature {
public:
    virtual ~Feature() = default;
};

class Features {
public:
    std::vector<Feature*> list;

public:
    Features();
    ~Features();

    template <typename T = Feature>
    void AddFeature(T** featurePtr)
    {
        *featurePtr = new T();
        this->list.push_back(*featurePtr);
    }

    template <typename T = Feature>
    void RemoveFeature(T** featurePtr)
    {
        this->list.erase(*featurePtr);
    }

    void DeleteAll();
};
