#pragma once

#include "ecs/System.hpp"
#include "ecs/World.hpp"

#include "visu/Components/RigideBodyComponent.hpp"
#include "visu/Components/TransformComponent.hpp"


class MotionSystem : public ee::ecs::System {
    public:
    void update(ee::ecs::World &_world, float _dt) override;
};

inline void MotionSystem::update(ee::ecs::World& _world, float _dt) {
    for (ee::ecs::EntityID e : m_entities){
        RigideBodyComponent& rb = _world.getComponent<RigideBodyComponent>(e);
        TransformComponent& tf =  _world.getComponent<TransformComponent>(e);

        tf.position += rb.velocity * _dt;
    }
}