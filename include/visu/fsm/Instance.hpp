#pragma once

#include <vector>

#include "visu/fsm/Params.hpp"

namespace ee::fsm {

    struct StateMachineDef;

    struct StateMachineInstance{
        const StateMachineDef* initializedWith = nullptr;
        int currentState = -1;
        float stateTime = 0.f;
        std::vector<ParamValue> params;
    };

}
