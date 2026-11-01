#pragma once

#include "visu/fsm/Condition.hpp"

namespace ee::fsm{
    struct TransitionDef{
        int targetState;
        bool allowSelf = false;
        std::vector<ConditionDef> conditions;
    };
    
    
}