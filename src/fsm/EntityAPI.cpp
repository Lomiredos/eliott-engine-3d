#include "visu/fsm/EntityAPI.hpp"

#include "visu/fsm/API.hpp"
#include "visu/fsm/FsmStore.hpp"
#include "visu/fsm/Utils.hpp"

namespace {

    StateMachineComponent* ready(ee::ecs::World &_world, ee::ecs::EntityID _entity){
        if (!_world.hasComponent<StateMachineComponent>(_entity)) return nullptr;
        StateMachineComponent* comp = _world.getComponent<StateMachineComponent>(_entity);
        return comp && ee::fsm::syncStateMachine(*comp) ? comp : nullptr;
    }

}

bool ee::fsm::syncStateMachine(StateMachineComponent &_comp){
    std::shared_ptr<const StateMachineDef> def = getStateMachine(_comp.path);
    if (def != _comp.def) _comp.def = std::move(def);
    if (!_comp.def) return false;
    if (_comp.inst.initializedWith != _comp.def.get()) initInstance(*_comp.def, _comp.inst);
    return true;
}

void ee::fsm::setBool(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name, bool _value){
    if (StateMachineComponent* c = ready(_world, _entity)) setBool(*c->def, c->inst, _name, _value);
}

void ee::fsm::setInt(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name, int _value){
    if (StateMachineComponent* c = ready(_world, _entity)) setInt(*c->def, c->inst, _name, _value);
}

void ee::fsm::setFloat(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name, float _value){
    if (StateMachineComponent* c = ready(_world, _entity)) setFloat(*c->def, c->inst, _name, _value);
}

void ee::fsm::setTrigger(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name){
    if (StateMachineComponent* c = ready(_world, _entity)) setTrigger(*c->def, c->inst, _name);
}

void ee::fsm::resetTrigger(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name){
    if (StateMachineComponent* c = ready(_world, _entity)) resetTrigger(*c->def, c->inst, _name);
}

bool ee::fsm::getBool(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name){
    StateMachineComponent* c = ready(_world, _entity);
    return c ? getBool(*c->def, c->inst, _name) : false;
}

int ee::fsm::getInt(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name){
    StateMachineComponent* c = ready(_world, _entity);
    return c ? getInt(*c->def, c->inst, _name) : 0;
}

float ee::fsm::getFloat(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name){
    StateMachineComponent* c = ready(_world, _entity);
    return c ? getFloat(*c->def, c->inst, _name) : 0.f;
}

bool ee::fsm::getTrigger(ee::ecs::World &_world, ee::ecs::EntityID _entity, const std::string &_name){
    StateMachineComponent* c = ready(_world, _entity);
    return c ? getTrigger(*c->def, c->inst, _name) : false;
}

std::string ee::fsm::currentStateName(ee::ecs::World &_world, ee::ecs::EntityID _entity){
    StateMachineComponent* c = ready(_world, _entity);
    if (!c || c->inst.currentState < 0 || c->inst.currentState >= (int)c->def->states.size()) return "";
    return c->def->states[c->inst.currentState].name;
}
