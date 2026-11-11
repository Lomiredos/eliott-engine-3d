#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"
#include "visu/fsm/Instance.hpp"

#include <memory>
#include <string>

struct StateMachineComponent
{
    std::string path;

    std::shared_ptr<const ee::fsm::StateMachineDef> def;
    ee::fsm::StateMachineInstance inst;
};

template <>
struct ee::reflection::Reflect<StateMachineComponent>
{
    static constexpr const char *name = "StateMachineComponent";

    static void visit(StateMachineComponent &_c, ee::reflection::FieldVisitor &_v)
    {
        _v.visit("FSM Path", _c.path);
    }
};
