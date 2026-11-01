#pragma once
#include "StateMachine.hpp"
#include <memory>
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


    void update(const StateMachineDef &_def,
                Ctx &_ctx,
                StateMachineInstance& _inst,
                float _dt);
    
} //namespace ee::fsm