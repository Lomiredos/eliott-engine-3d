#pragma once

// ---------------------------------------------------------------------------
// Hote de systemes cote World typé (opt-in, game-only comme WorldLoader).
//
// Le jeu enregistre ses TYPES de systemes par nom (makeSystemFactory<T>()).
// build() lit systems.json (deja trié par priorité), et pour chaque systeme :
// l'instancie dans le World (registerSystem<T> + setSystemSignature depuis les
// noms de composants) -> le SystemManager d'eliott-ecs remplit tout seul son
// m_entities. updateAll() appelle update() dans l'ordre de priorité.
//
// -> plus de scan : chaque systeme itere SA liste d'entités.
// ---------------------------------------------------------------------------

#include "ecs/World.hpp"
#include "ecs/System.hpp"

#include "visu/core/SystemInfo.hpp"
#include "visu/scene/WorldLoader.hpp" // WorldRegistry (signatures)

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ee::scene
{
    // Instancie un systeme de type T dans le World avec sa signature.
    using SystemFactory = std::function<std::shared_ptr<ee::ecs::System>(
        ee::ecs::World &, const WorldRegistry &, const std::vector<std::string> &)>;

    template <typename T>
    SystemFactory makeSystemFactory()
    {
        return [](ee::ecs::World &_w, const WorldRegistry &_reg,
                  const std::vector<std::string> &_sigNames)
        {
            std::shared_ptr<T> s = _w.registerSystem<T>();
            _w.setSystemSignature<T>(_reg.signatureOf(_sigNames));
            return std::static_pointer_cast<ee::ecs::System>(s);
        };
    }

    class SystemHost
    {
    public:
        // Associe un nom de systeme a son type C++.
        void reg(const std::string &_name, SystemFactory _factory)
        {
            m_factories[_name] = std::move(_factory);
        }

        // Instancie les systemes de _schedule (trié par priorité) dans le World.
        void build(ee::ecs::World &_world, const WorldRegistry &_reg,
                   const std::vector<SystemInfo> &_schedule)
        {
            for (const SystemInfo &s : _schedule)
            {
                auto it = m_factories.find(s.name);
                if (it == m_factories.end())
                    continue; // systeme authoré mais pas enregistré cote code
                m_ordered.push_back(it->second(_world, _reg, s.components));
            }
        }

        // Appelle update() de chaque systeme, dans l'ordre de build (priorité).
        void updateAll(ee::ecs::World &_world, float _dt)
        {
            for (auto &sys : m_ordered)
                sys->update(_world, _dt);
        }

    private:
        std::unordered_map<std::string, SystemFactory> m_factories;
        std::vector<std::shared_ptr<ee::ecs::System>> m_ordered;
    };
}
