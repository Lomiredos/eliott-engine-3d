#pragma once

#include <any>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Bus d'evenements global : on s'abonne a un type, on publie une valeur de ce
// type, tous les abonnes de ce type sont appeles.
//
//   struct PlayerDied { int id; };
//   ee::core::EventBus::getInstance().subscribe<PlayerDied>(
//       [](PlayerDied _e) { ... });
//   ee::core::EventBus::getInstance().publish(PlayerDied{3});
// ---------------------------------------------------------------------------
namespace ee::core
{
    class EventBus
    {
    private:
        std::unordered_map<std::type_index, std::vector<std::any>> m_listeners;

        EventBus() {}

    public:
        static EventBus &getInstance()
        {
            static EventBus instance = EventBus();
            return instance;
        }

        template <typename T>
        void publish(T _data)
        {
            auto it = m_listeners.find(typeid(T));
            if (it != m_listeners.end())
            {
                for (auto &cb : it->second)
                    std::any_cast<std::function<void(T)>>(cb)(_data);
            }
        }

        template <typename T>
        void subscribe(std::function<void(T)> _callback)
        {
            m_listeners[std::type_index(typeid(T))].push_back(_callback);
        }

        // Vide toutes les souscriptions — a appeler avant un changement de scene
        void clearAll()
        {
            m_listeners.clear();
        }
    };
} // namespace ee::core
