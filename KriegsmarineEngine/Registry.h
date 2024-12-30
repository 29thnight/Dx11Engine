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
        // ������Ʈ �����ڸ� ��������, �� �������� dense �迭���� ��ȿ�� ��ƼƼ�� ����
        std::vector<std::vector<Entity>> entityLists;

        // ������Ʈ���� ��ƼƼ ID ����
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

        // ���� ��ƼƼ ����Ʈ�� ��� �ִٸ� ����� ��� ����
        if (entityLists.empty()) return {};

        // ��ƼƼ ����Ʈ�� ũ�� ������ ���� (���� ���� ��ƼƼ�� ���� ������Ʈ���� ó��)
        std::sort(entityLists.begin(), entityLists.end(), [](const auto& a, const auto& b)
            {
                return a.size() < b.size();
            });

        // ù ��° ����Ʈ�� �������� ������ ���
        std::vector<Entity> result = entityLists[0];
        for (size_t i = 1; i < entityLists.size(); ++i)
        {
            std::vector<Entity> temp;
            std::set_intersection(result.begin(), result.end(),
                entityLists[i].begin(), entityLists[i].end(), std::back_inserter(temp)
            );
            result = std::move(temp);

            // �������� ��� �� �̻� ó���� �ʿ� ����
            if (result.empty()) break;
        }

        return result;
    }

private:
    EntityManager m_entityManager;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentManager>> m_componentManagers;

};
