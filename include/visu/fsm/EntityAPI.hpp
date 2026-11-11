#pragma once

#include <string>

#include "ecs/World.hpp"
#include "visu/components/StateMachineComponent.hpp"

namespace ee::fsm {

    bool syncStateMachine(StateMachineComponent &_comp);

    void setBool(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name, bool _value);
    void setInt(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name, int _value);
    void setFloat(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name, float _value);
    void setTrigger(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name);
    void resetTrigger(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name);

    bool  getBool(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name);
    int   getInt(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name);
    float getFloat(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name);
    bool  getTrigger(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name);

    std::string currentStateName(ee::ecs::World &_world, ee::ecs::EntityID _entity);

}
