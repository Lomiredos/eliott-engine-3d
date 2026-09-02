#pragma once

#include "visu/core/SystemInfo.hpp"

#include "visu/scene/SystemHost.hpp"

#include "visu/systems/GravitySystem.hpp"
#include "visu/systems/MotionSystem.hpp"

#include <vector>

namespace ee::scene {

    inline void registerEngineSystems(SystemHost &_host){
        _host.reg("GravitySystem", makeSystemFactory<GravitySystem>());
        _host.reg("MotionSystem", makeSystemFactory<MotionSystem>());
    }

    inline std::vector<SystemInfo> engineSystemInfos() {
        std::vector<SystemInfo> infos;
        infos.push_back({"GravitySystem", {"RigideBodyComponent"}, 0, "engine"});
        infos.push_back({"MotionSystem", {"TransformComponent","RigideBodyComponent"}, 100, "engine"});

        return infos;
    }

}