#pragma once

#include "ecs/World.hpp"
#include "ecs/ComponentRegistry.hpp"

#include "visu/core/SceneInfo.hpp"
#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/SceneBind.hpp"

#include "visu/components/Components.hpp"

#include <algorithm>
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
            // Pour le lien avec l'editeur (inspecteur en direct) :
            std::function<bool(ee::ecs::World &, ee::ecs::EntityID)> has;
            std::function<ComponentInstance(ee::ecs::World &, ee::ecs::EntityID)> save;
            std::function<void(ee::ecs::World &, ee::ecs::EntityID, const ComponentInstance &)> patch;
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
            e.has = [](ee::ecs::World &_w, ee::ecs::EntityID _id)
            { return _w.hasComponent<T>(_id); };
            e.save = [](ee::ecs::World &_w, ee::ecs::EntityID _id)
            { return ee::reflection::saveToInstance<T>(*_w.getComponent<T>(_id)); };
            e.patch = [](ee::ecs::World &_w, ee::ecs::EntityID _id, const ComponentInstance &_ci)
            {
                if (_w.hasComponent<T>(_id))
                    ee::reflection::patchFromInstance<T>(*_w.getComponent<T>(_id), _ci);
            };

            std::string name = ee::reflection::Reflect<T>::name;
            if (m_entries.find(name) == m_entries.end())
                m_order.push_back(name);
            m_entries[name] = std::move(e);
        }

        // Noms des composants, dans l'ordre d'enregistrement (stable).
        const std::vector<std::string> &names() const { return m_order; }

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
        std::vector<std::string> m_order;
    };

    inline void registerEngineComponents(WorldRegistry &_reg)
    {
        _reg.reg<TransformComponent>();
        _reg.reg<SphereComponent>();
        _reg.reg<RectComponent>();
        _reg.reg<CapsuleComponent>();
        _reg.reg<MeshComponent>();
        _reg.reg<RigideBodyComponent>();
        _reg.reg<ColliderComponent>();
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

    // Etat courant du World sous forme de valeurs dynamiques (meme format que
    // la scene) : une entree par entite de _ents, dans le meme ordre, donc le
    // meme index que dans la scene d'origine. Les composants suivent l'ordre
    // de la scene, puis ceux ajoutes en jeu (ordre d'enregistrement).
    inline SceneInfo snapshotWorld(ee::ecs::World &_world,
                                   const std::vector<ee::ecs::EntityID> &_ents,
                                   const SceneInfo &_scene,
                                   const WorldRegistry &_reg)
    {
        SceneInfo out;
        out.name = _scene.name;
        out.entities.reserve(_ents.size());
        for (size_t i = 0; i < _ents.size(); ++i)
        {
            ee::ecs::EntityID id = _ents[i];
            EntityInfo ent;
            ent.name = i < _scene.entities.size() ? _scene.entities[i].name : std::string();

            std::vector<std::string> order;
            if (i < _scene.entities.size())
                for (const ComponentInstance &ci : _scene.entities[i].components)
                    order.push_back(ci.name);
            for (const std::string &n : _reg.names())
                if (std::find(order.begin(), order.end(), n) == order.end())
                    order.push_back(n);

            for (const std::string &n : order)
                if (const WorldRegistry::Entry *e = _reg.find(n))
                    if (e->has(_world, id))
                        ent.components.push_back(e->save(_world, id));
            out.entities.push_back(std::move(ent));
        }
        return out;
    }
}
