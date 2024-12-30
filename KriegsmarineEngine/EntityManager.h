#pragma once
#include "Core.Definition.h"
#include <cstdint>
#include <queue>

constexpr Entity INVALID_ENTITY = static_cast<Entity>(-1);

class EntityManager final
{
public:
    Entity CreateEntity();
    void DestroyEntity(Entity entity);

private:
    Entity m_nextEntity{ 0 };
    std::queue<Entity> m_availableEntity;
};
