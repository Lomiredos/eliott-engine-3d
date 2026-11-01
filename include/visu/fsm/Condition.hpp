#pragma once
#include <variant>

#include "visu/fsm/Operation.hpp"
#include "visu/fsm/Context.hpp"
#include "visu/fsm/Params.hpp"

namespace ee::fsm {

    using ConditionFn = bool (*)(const Ctx&); // un pointeur vers une fonction qui return un bool et prend un const Ctx& en arg


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

}