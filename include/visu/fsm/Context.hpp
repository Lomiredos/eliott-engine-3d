#pragma once
#include "ecs/World.hpp"


namespace ee::fsm{
    struct Ctx{
        ee::ecs::World&   world; 
        ee::ecs::EntityID entity;
    };

}