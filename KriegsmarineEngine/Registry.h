#pragma once
#include "Core.Definition.h"
#include "EntityManager.h"
#include "ComponentManager.h"
#include <typeindex>

//https://en.cppreference.com/w/cpp/algorithm/set_intersection
//https://en.cppreference.com/w/cpp/types/type_index
class Registry
{
public:
    Entity CreateEntity()
    {
        return m_entityManager.CreateEntity();
    }

    void DestroyEntity(Entity entity)
    {
        m_entityManager.DestroyEntity(entity);
        for (auto& [type, manager] : m_componentManagers)
        {
            manager->Remove(entity);
        }
    }

    template <typename Component, typename... Args>
    void AddComponent(Entity entity, Args&&... args)
    {
        auto& manager = GetOrCreateComponentManager<Component>();
        manager.Add(entity, Component{ std::forward<Args>(args)... });
    }

    template <typename Component>
    Component* GetComponent(Entity entity)
    {
        return GetOrCreateComponentManager<Component>().Get(entity);
    }

    template <typename Component>
    void RemoveComponent(Entity entity)
    {
        auto& manager = GetComponentManager<Component>();
        manager.Remove(entity);
    }

    template <typename... Components, typename Func>
    void View(Func&& func)
    {
        std::vector<Entity> entities = GetEntitiesWithComponents<Components...>();
        for (auto entity : entities)
        {
            func(entity, GetComponent<Components>(entity)...);
        }
    }

private:
    template <typename Component>
    ComponentManager<Component>& GetOrCreateComponentManager()
    {
        auto type = std::type_index(typeid(Component));
        if (m_componentManagers.find(type) == m_componentManagers.end())
        {
            m_componentManagers[type] = std::make_unique<ComponentManager<Component>>();
        }
        return *static_cast<ComponentManager<Component>*>(m_componentManagers[type].get());
    }

    template <typename... Components>
    std::vector<Entity> GetEntitiesWithComponents()
    {
        // 컴포넌트 관리자를 가져오고, 각 관리자의 dense 배열에서 유효한 엔티티를 수집
        std::vector<std::vector<Entity>> entityLists;

        // 컴포넌트별로 엔티티 ID 수집
        ([&]() {
            const auto& manager = GetOrCreateComponentManager<Components>();
            std::vector<Entity> entities;
            for (size_t i = 0; i < manager.GetDense().size(); ++i)
            {
                if (manager.GetDense()[i] != -1)
                {
                    entities.push_back(static_cast<Entity>(i));
                }
            }
            entityLists.push_back(std::move(entities));
            }(), ...);

        // 만약 엔티티 리스트가 비어 있다면 결과도 비어 있음
        if (entityLists.empty()) return {};

        // 엔티티 리스트를 크기 순으로 정렬 (가장 적은 엔티티를 가진 컴포넌트부터 처리)
        std::sort(entityLists.begin(), entityLists.end(), [](const auto& a, const auto& b)
            {
                return a.size() < b.size();
            });

        // 첫 번째 리스트를 기준으로 교집합 계산
        std::vector<Entity> result = entityLists[0];
        for (size_t i = 1; i < entityLists.size(); ++i)
        {
            std::vector<Entity> temp;
            std::set_intersection(result.begin(), result.end(),
                entityLists[i].begin(), entityLists[i].end(), std::back_inserter(temp)
            );
            result = std::move(temp);

            // 교집합이 비면 더 이상 처리할 필요 없음
            if (result.empty()) break;
        }

        return result;
    }

private:
    EntityManager m_entityManager;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentManager>> m_componentManagers;

};
