#pragma once

#include "ecs/System.hpp"
#include "ecs/World.hpp"

#include "visu/components/RigidBodyComponent.hpp"

class GravitySystem : public ee::ecs::UpdateSystem {
    public:
    void update(ee::ecs::World& _world, float _dt) override;
};

inline void GravitySystem::update(ee::ecs::World& _world, float _dt) {


    const float g = -9.81f;

    for (ee::ecs::EntityID e : m_entities){
        RigidBodyComponent& rb = *_world.getComponent<RigidBodyComponent>(e);
        if (rb.isStatic)
            continue;

        rb.velocity.y += g * _dt;
    }
};