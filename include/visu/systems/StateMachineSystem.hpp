#pragma once

#include "ecs/System.hpp"
#include "ecs/World.hpp"

#include "visu/components/StateMachineComponent.hpp"
#include "visu/fsm/EntityAPI.hpp"
#include "visu/fsm/StateMachine.hpp"
#include "visu/fsm/Utils.hpp"

#include <memory>
#include <vector>

class StateMachineSystem : public ee::ecs::UpdateSystem {
    public:
    void update(ee::ecs::World& _world, float _dt) override;
};

inline void StateMachineSystem::update(ee::ecs::World& _world, float _dt) {
    const std::vector<ee::ecs::EntityID> entities(m_entities.begin(), m_entities.end());

    for (ee::ecs::EntityID e : entities){
        if (!_world.hasComponent<StateMachineComponent>(e))
            continue;
        StateMachineComponent& comp = *_world.getComponent<StateMachineComponent>(e);
        if (!ee::fsm::syncStateMachine(comp))
            continue;

        std::shared_ptr<const ee::fsm::StateMachineDef> def = comp.def;
        ee::fsm::Ctx ctx{_world, e, *def, comp.inst};
        ee::fsm::update(*def, ctx, comp.inst, _dt);
    }
}
