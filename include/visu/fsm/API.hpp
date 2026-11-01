#pragma once
#include "visu/fsm/StateMachine.hpp"


namespace ee::fsm{

    void setBool(const StateMachineDef &_def, StateMachineInstance &_inst, const std::string& _name, bool _value);
    void setInt(const StateMachineDef &_def, StateMachineInstance &_inst, const std::string& _name, int _value);
    void setFloat(const StateMachineDef &_def, StateMachineInstance &_inst, const std::string& _name, float _value);
    void setTrigger(const StateMachineDef &_def, StateMachineInstance &_inst, const std::string& _name);
    void resetTrigger(const StateMachineDef &_def, StateMachineInstance &_inst, const std::string& _name);
    
    void setBool(const StateMachineDef &_def, StateMachineInstance &_inst, int _index, bool _value);
    void setInt(const StateMachineDef &_def, StateMachineInstance &_inst, int _index, int _value);
    void setFloat(const StateMachineDef &_def, StateMachineInstance &_inst, int _index, float _value);
    void setTrigger(const StateMachineDef &_def, StateMachineInstance &_inst, int _index);
    void resetTrigger(const StateMachineDef &_def, StateMachineInstance &_inst, int _index);


    bool  getBool(const StateMachineDef &_def, const StateMachineInstance &_inst, const std::string& _name);
    float getFloat(const StateMachineDef &_def, const StateMachineInstance &_inst, const std::string& _name);
    int   getInt(const StateMachineDef &_def, const StateMachineInstance &_inst, const std::string& _name);
    bool  getTrigger(const StateMachineDef &_def, const StateMachineInstance &_inst, const std::string& _name);

    bool  getBool(const StateMachineDef &_def, const StateMachineInstance &_inst, int _index);
    float getFloat(const StateMachineDef &_def, const StateMachineInstance &_inst, int _index);
    int   getInt(const StateMachineDef &_def, const StateMachineInstance &_inst, int _index);
    bool  getTrigger(const StateMachineDef &_def, const StateMachineInstance &_inst, int _index);

}