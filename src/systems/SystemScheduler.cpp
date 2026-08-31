#include "visu/systems/SystemScheduler.hpp"

#include <iostream>

namespace ee::systems
{
    void SystemRegistry::add(const std::string &_name, SystemFn _fn)
    {
        m_fns[_name] = std::move(_fn);
    }

    const SystemFn *SystemRegistry::find(const std::string &_name) const
    {
        auto it = m_fns.find(_name);
        return it == m_fns.end() ? nullptr : &it->second;
    }

    void runSystems(const std::vector<SystemInfo> &_schedule,
                    const SystemRegistry &_registry,
                    const FrameContext &_ctx)
    {
        for (const SystemInfo &sys : _schedule)
        {
            const SystemFn *fn = _registry.find(sys.name);
            if (!fn)
            {
                // Systeme authore (systems.json) mais pas encore code/enregistre.
                static std::unordered_map<std::string, bool> warned;
                if (!warned[sys.name])
                {
                    std::cerr << "[scheduler] systeme '" << sys.name
                              << "' non enregistre (ignore)\n";
                    warned[sys.name] = true;
                }
                continue;
            }
            (*fn)(_ctx);
        }
    }
}
