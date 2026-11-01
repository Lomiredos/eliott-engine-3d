#pragma once

#include <vector>

#include "visu/fsm/State.hpp"
#include "visu/fsm/Params.hpp"

namespace ee::fsm {    
    
    struct StateMachineDef {
        std::vector<StateDef> states;
        std::vector<TransitionDef> anyTransition;
        std::vector<ParamDef> params;
        int entryState = 0;
    };

    struct StateMachineInstance{
        const StateMachineDef* initializedWith = nullptr;
        int currentState = -1;
        float stateTime = 0.f;
        std::vector<ParamValue> params;
    };

}