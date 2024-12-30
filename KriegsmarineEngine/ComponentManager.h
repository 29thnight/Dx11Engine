#pragma once
#include "Core.Definition.h"
#include "IComponentManager.h"

//https://www.geeksforgeeks.org/sparse-set/
//sparse set을 이용하여 entity를 관리한다.
template <typename Component>
class ComponentManager : public IComponentManager
{
public:
    void Add(Entity entity, const Component& component)
    {
        if (entity >= m_components.size())
        {
            dense.resize(entity + 1, -1);
        }
        dense[entity] = static_cast<int>(m_components.size());
        m_components.push_back(component);
    }

    void Remove(Entity entity) override
    {
        if (entity < dense.size() && -1 != dense[entity])
        {
            int index = dense[entity];
            m_components[index] = m_components.back();
            m_components.pop_back();
            dense[entity] = -1;
        }
    }

    Component* Get(Entity entity)
    {
        if (entity < dense.size() && -1 != dense[entity])
        {
            return &m_components[dense[entity]];
        }
        return nullptr;
    }

    const std::vector<int>& GetDense() const { return dense; } // Dense 배열 반환
    const std::vector<Component>& GetComponents() const { return m_components; } // Component 배열 반환

private:
    std::vector<int> dense;
    std::vector<Component> m_components;
};
