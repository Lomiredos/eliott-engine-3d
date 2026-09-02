#pragma once

// ---------------------------------------------------------------------------
// Chargeur scene (donnees dynamiques) -> World typé (eliott-ecs).
//
// Header-only et OPT-IN : seul le JEU l'inclut. ee-core.lib n'est donc PAS
// couplee a eliott-ecs (l'editeur, qui reste dynamique, ne voit jamais ce
// fichier). Ici on marie les deux mondes : ee-core (reflexion + composants
// moteur) et eliott-ecs (World).
//
// Un WorldRegistry mappe un NOM de composant vers :
//   - un "add" : instancie la struct typée depuis le ComponentInstance (via la
//     reflexion) et l'ajoute au World ;
//   - un "componentId" : le bit de signature (getComponentID<T>).
// -> permet de desérialiser une scene ET de construire les signatures des
//    systemes a partir des noms de systems.json.
// ---------------------------------------------------------------------------

#include "ecs/World.hpp"
#include "ecs/ComponentRegistry.hpp" // getComponentID, Signature, EntityID

#include "visu/core/SceneInfo.hpp"
#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/SceneBind.hpp"

// Composants MOTEUR (leur struct + Reflect).
#include "visu/components/TransformComponent.hpp"
#include "visu/components/SphereComponent.hpp"
#include "visu/components/RectComponent.hpp"
#include "visu/components/CapsuleComponent.hpp"
#include "visu/components/MeshComponent.hpp"

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

        // Enregistre un composant typé T (doit specialiser Reflect<T>).
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

        // Signature (bitset) a partir d'une liste de noms de composants.
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

    // Enregistre les composants MOTEUR (Transform + formes).
    inline void registerEngineComponents(WorldRegistry &_reg)
    {
        _reg.reg<TransformComponent>();
        _reg.reg<SphereComponent>();
        _reg.reg<RectComponent>();
        _reg.reg<CapsuleComponent>();
        _reg.reg<MeshComponent>();
    }

    // Instancie la scene authoree dans un World typé. Renvoie les EntityID crees.
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
