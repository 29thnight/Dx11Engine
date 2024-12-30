#pragma once
#include "Core.Definition.h"

interface IComponentManager
{
    virtual ~IComponentManager() = default;
    virtual void Remove(Entity entity) = 0;
};
