#pragma once

#include <string>

#include "visu/fsm/StateMachine.hpp"


namespace ee::fsm{


    int findParam(const StateMachineDef &_def, const std::string &_name);
    void update(const StateMachineDef &_def,
                Ctx &_ctx,
                StateMachineInstance& _inst,
                float _dt);
    void initInstance(const StateMachineDef &_def, StateMachineInstance &_inst);
    
}