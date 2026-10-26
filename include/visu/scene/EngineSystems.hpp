#pragma once

#include "visu/core/SystemInfo.hpp"

#include "visu/scene/SystemHost.hpp"
#include "visu/scene/EngineSystemsData.hpp" // genere par CMake depuis les .json

#include "visu/systems/GravitySystem.hpp"
#include "visu/systems/MotionSystem.hpp"
#include "visu/systems/CollisionSystem.hpp"

#include <optional>
#include <vector>

namespace ee::scene {

    inline void registerEngineSystems(SystemHost &_host){
        _host.reg("GravitySystem", makeSystemFactory<GravitySystem>());
        _host.reg("MotionSystem", makeSystemFactory<MotionSystem>());
        _host.reg("CollisionSystem", makeSystemFactory<CollisionSystem>());
    }

    // Signature + priorite des systemes moteur : lues depuis leurs sidecars
    // include/visu/systems/<Nom>.json (meme format que les systemes du projet),
    // embarques dans le binaire au build -> pas de fichier a cote de l'exe.
    inline std::vector<SystemInfo> engineSystemInfos() {
        std::vector<SystemInfo> infos;
        for (const char *const *json = kEngineSystemJsons; *json; ++json)
            if (std::optional<SystemInfo> info = parseSystemInfo(*json))
                infos.push_back(*info);
        return infos;
    }

}
