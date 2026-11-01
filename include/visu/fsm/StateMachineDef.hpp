#pragma once
#include "StateMachine.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <variant>
#include <string>

namespace ee::fsm {

    enum class Op {Eq, Neq, Lt, Gt, Le, Ge};
    enum class ParamType {Bool, Float, Int, Trigger};

    using ParamValue = std::variant<bool, float, int>;
    using ConditionFn = bool (*)(const Ctx&); // un pointeur vers une fonction qui return un bool et prend un const Ctx& en arg


    struct ParamDef{
        std::string name;
        ParamType type;
        ParamValue defaultValue;
    };

    struct ParamCondition {
        int param;
        Op op;
        ParamValue value;
    };

    struct StateTimeCondition{
        Op op;
        float value;
    };

    struct CustomCondition {
        ConditionFn fn;
        bool expected;
    };

    using ConditionDef = std::variant<ParamCondition, StateTimeCondition, CustomCondition>;

    struct TransitionDef{
        int targetState;
        bool allowSelf = false;
        std::vector<ConditionDef> conditions;
    };
    
    struct StateDef{
        std::string name;
        std::unique_ptr<State> state;
        std::vector<TransitionDef> transition;
    };
    
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


    class ConditionRegistry{
        std::unordered_map<std::string, ConditionFn> m_functions;
        
    public:
        void reg(const std::string& _name, ConditionFn _cfn);
        ConditionFn find(const std::string &_name) const;
        
    };

    int findParam(const StateMachineDef &_def, const std::string &_name);

    void update(const StateMachineDef &_def,
                Ctx &_ctx,
                StateMachineInstance& _inst,
                float _dt);

    void initInstance(const StateMachineDef &_def, StateMachineInstance &_inst);
    
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

} //namespace ee::fsm