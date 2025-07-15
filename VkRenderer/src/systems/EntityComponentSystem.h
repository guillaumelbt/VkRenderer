#pragma once
#include <unordered_map>
#include <typeindex>
#include <any>
#include <memory>

using Entity = unsigned int;

class EntityComponentSystem 
{
    std::unordered_map<std::type_index, std::unordered_map<Entity, std::any>> componentStores;

public:
    Entity CreateEntity() 
    {
        static Entity nextId = 0;
        return nextId++;
    }
    void DestroyEntity(Entity _id)
    {
        for (auto& [type, store] : componentStores) 
        {
            store.erase(_id);
        }
    }

    template<typename Component>
    void AddComponent(Entity _id, Component _component) 
    {
        componentStores[typeid(Component)][_id] = std::move(_component);
    }

    template<typename Component>
    bool HasComponent(Entity _id) 
    {
        auto it = componentStores.find(typeid(Component));
        if (it == componentStores.end()) return false;
        return it->second.count(_id) > 0;
    }

    template<typename Component>
    Component& GetComponent(Entity _id) 
    {
        return std::any_cast<Component&>(componentStores[typeid(Component)][_id]);
    }

    template<typename Component>
    void RemoveComponent(Entity _id) {
        componentStores[typeid(Component)].erase(_id);
    }

    template<typename Component, typename Func>
    void ForEach(Func _func) 
    {
        auto it = componentStores.find(typeid(Component));
        if (it == componentStores.end()) return;
        for (auto& [id, anyComp] : it->second) {
            _func(id, std::any_cast<Component&>(anyComp));
        }
    }

    template<typename Component1, typename Component2, typename Func>
    void ForEach(Func _func) 
    {
        auto it1 = componentStores.find(typeid(Component1));
        auto it2 = componentStores.find(typeid(Component2));
        if (it1 == componentStores.end() || it2 == componentStores.end()) return;
        
        for (auto& [id, anyComp1] : it1->second) {
            auto it2_comp = it2->second.find(id);
            if (it2_comp != it2->second.end()) {
                _func(id, std::any_cast<Component1&>(anyComp1), std::any_cast<Component2&>(it2_comp->second));
            }
        }
    }

    size_t GetEntityCount() const 
    {
        size_t maxEntity = 0;
        for (const auto& [type, store] : componentStores) {
            for (const auto& [entity, component] : store) {
                maxEntity = std::max(maxEntity, static_cast<size_t>(entity));
            }
        }
        return maxEntity + 1;
    }
}; 