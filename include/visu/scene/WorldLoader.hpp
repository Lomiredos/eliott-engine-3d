#pragma once

#include "ecs/World.hpp"
#include "ecs/ComponentRegistry.hpp"

#include "visu/core/SceneInfo.hpp"
#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/SceneBind.hpp"

#include "visu/components/Components.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ee::scene
{
    class WorldRegistry
    {
    public:
        struct Entry
        {
            std::function<void(ee::ecs::World &, ee::ecs::EntityID, const ComponentInstance &)> add;
            std::function<size_t()> componentId;
        };

        template <typename T>
        void reg()
        {
            Entry e;
            e.add = [](ee::ecs::World &_w, ee::ecs::EntityID _id, const ComponentInstance &_ci)
            {
                _w.addComponent<T>(_id, ee::reflection::loadFromInstance<T>(_ci));
            };
            e.componentId = []()
            { return ee::ecs::getComponentID<T>(); };
            m_entries[ee::reflection::Reflect<T>::name] = std::move(e);
        }

        const Entry *find(const std::string &_name) const
        {
            auto it = m_entries.find(_name);
            return it == m_entries.end() ? nullptr : &it->second;
        }

        ee::ecs::Signature signatureOf(const std::vector<std::string> &_names) const
        {
            ee::ecs::Signature sig;
            for (const std::string &n : _names)
                if (const Entry *e = find(n))
                    sig.set(e->componentId());
            return sig;
        }

    private:
        std::unordered_map<std::string, Entry> m_entries;
    };

    inline void registerEngineComponents(WorldRegistry &_reg)
    {
        _reg.reg<TransformComponent>();
        _reg.reg<SphereComponent>();
        _reg.reg<RectComponent>();
        _reg.reg<CapsuleComponent>();
        _reg.reg<MeshComponent>();
        _reg.reg<RigideBodyComponent>();
    }

    inline std::vector<ee::ecs::EntityID> loadSceneIntoWorld(ee::ecs::World &_world,
                                                             const SceneInfo &_scene,
                                                             const WorldRegistry &_reg)
    {
        std::vector<ee::ecs::EntityID> created;
        for (const EntityInfo &ent : _scene.entities)
        {
            ee::ecs::EntityID id = _world.createEntity();
            created.push_back(id);
            for (const ComponentInstance &ci : ent.components)
                if (const WorldRegistry::Entry *e = _reg.find(ci.name))
                    e->add(_world, id, ci);
        }
        _world.flush(); // active les entites -> peuple les listes des systemes
        return created;
    }
}
