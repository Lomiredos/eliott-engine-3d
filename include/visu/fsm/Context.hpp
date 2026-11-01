#pragma once
#include "ecs/World.hpp"
namespace ee::fsm{

    struct StateMachineDef;
    struct StateMachineInstance;

    struct Ctx{
        ee::ecs::World&   world; 
        ee::ecs::EntityID entity;
        const ee::fsm::StateMachineDef& def;
        ee::fsm::StateMachineInstance& inst;
    };

}