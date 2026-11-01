#pragma once

#include "ecs/World.hpp"

namespace ee::fsm {

    struct Ctx{
        ee::ecs::World&   world; 
        ee::ecs::EntityID entity;
    };

    class State
    {
    public:
        virtual ~State() = default;
        virtual void OnStart(Ctx& _owner) = 0;
        virtual void OnUpdate(Ctx& _owner, float _dt) = 0; // no const because it may modifie owner
        virtual void OnEnd(Ctx& _owner) = 0;
    };

} //namespace ee::fsm