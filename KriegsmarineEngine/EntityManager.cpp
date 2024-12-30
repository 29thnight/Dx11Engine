#include "EntityManager.h"

Entity EntityManager::CreateEntity()
{
    if (!m_availableEntity.empty())
    {
        Entity entity = m_availableEntity.front();
        m_availableEntity.pop();
        return entity;
    }

    return m_nextEntity++;
}

void EntityManager::DestroyEntity(Entity entity)
{
    m_availableEntity.push(entity);
}
