#pragma once

#include <string>
#include <vector>

#include "visu/fsm/State.hpp"
#include "visu/fsm/Params.hpp"
#include "visu/fsm/Instance.hpp"

namespace ee::fsm {    
    
    struct StateMachineDef {
        std::string name;
        std::vector<StateDef> states;
        std::vector<TransitionDef> anyTransition;
        std::vector<ParamDef> params;
        int entryState = 0;
    };


}