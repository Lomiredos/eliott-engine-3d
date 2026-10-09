#pragma once

#include <cmath>
#include <vector>

#include "ecs/System.hpp"
#include "ecs/World.hpp"

#include "math/Quaternion.hpp"

#include "visu/components/ColliderComponent.hpp"
#include "visu/components/RigideBodyComponent.hpp"
#include "visu/components/TransformComponent.hpp"

struct Contact {
  bool hit = false;
  ee::math::Vector3<float> normal{0.0f, 0.0f, 0.0f};
  float depth = 0.0f;
};

inline Contact SphereToSphere(const ee::math::Vector3<float> &_centerA,
                              const ee::math::Vector3<float> &_centerB,
                              float _rA, float _rB) {
  float r = _rA + _rB;
  float d2 = _centerA.DistanceSquared(_centerB);
  if (d2 >= r * r)
    return {};

  float dist = std::sqrt(d2);

  if (dist == 0.0f)
    return {true, {0.0f, 1.0f, 0.0f}, r};

  ee::math::Vector3<float> normal = (_centerB - _centerA) / dist; // A -> B
  float depth = r - dist;
  return {true, normal, depth};
}

inline Contact SphereToBox(const ee::math::Vector3<float> &_sphereCenter,
                           float _r, const ee::math::Vector3<float> &_boxCenter,
                           const ee::math::Vector3<float> &_half,
                           const ee::math::Quaternion &_boxRot) {

  ee::math::Vector3<float> d = _sphereCenter - _boxCenter;
  ee::math::Vector3<float> local{d.Dot(_boxRot.right()), d.Dot(_boxRot.up()),
                                 d.Dot(_boxRot.back())};

  auto clampf = [](float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
  };
  ee::math::Vector3<float> closest{clampf(local.x, -_half.x, _half.x),
                                   clampf(local.y, -_half.y, _half.y),
                                   clampf(local.z, -_half.z, _half.z)};

  ee::math::Vector3<float> diff = local - closest;
  float d2 = diff.Dot(diff);

  if (d2 > _r * _r)
    return {};

  if (d2 > 1e-8f) {
    float dist = std::sqrt(d2);
    ee::math::Vector3<float> nLocal = (closest - local) / dist;
    return {true, _boxRot.rotate(nLocal), _r - dist};
  }

  float px = _half.x - std::fabs(local.x);
  float py = _half.y - std::fabs(local.y);
  float pz = _half.z - std::fabs(local.z);

  ee::math::Vector3<float> nLocal;
  float depth;
  if (px <= py && px <= pz) {
    nLocal = {local.x < 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f};
    depth = _r + px;
  } else if (py <= pz) {
    nLocal = {0.0f, local.y < 0.0f ? 1.0f : -1.0f, 0.0f};
    depth = _r + py;
  } else {
    nLocal = {0.0f, 0.0f, local.z < 0.0f ? 1.0f : -1.0f};
    depth = _r + pz;
  }
  return {true, _boxRot.rotate(nLocal), depth};
}

class CollisionSystem : public ee::ecs::UpdateSystem {
public:
  void update(ee::ecs::World &_world, float _dt) override;
};

inline void CollisionSystem::update(ee::ecs::World &_world, float _dt) {
  std::vector<ee::ecs::EntityID> ents(m_entities.begin(), m_entities.end());

  for (size_t i = 0; i < ents.size(); ++i) {
    ee::ecs::EntityID idA = ents[i];

    ColliderComponent &cc = *_world.getComponent<ColliderComponent>(idA);
    TransformComponent &tf = *_world.getComponent<TransformComponent>(idA);

    if (!cc.isActive)
      continue;

    RigideBodyComponent *rbA = nullptr;
    if (_world.hasComponent<RigideBodyComponent>(idA))
      rbA = _world.getComponent<RigideBodyComponent>(idA);

    for (size_t j = i + 1; j < ents.size(); ++j) {
      ee::ecs::EntityID idB = ents[j];

      ColliderComponent &occ = *_world.getComponent<ColliderComponent>(idB);
      TransformComponent &otf = *_world.getComponent<TransformComponent>(idB);

      if (!occ.isActive)
        continue;

      Contact info;
      if (cc.shape == ColliderShape::Sphere &&
          occ.shape == ColliderShape::Sphere)
        info = SphereToSphere(tf.position + cc.offset,
                              otf.position + occ.offset, cc.radius, occ.radius);
      else if (cc.shape == ColliderShape::Sphere &&
               occ.shape == ColliderShape::Box) {
        ee::math::Quaternion qB = ee::math::Quaternion::fromEulerDeg(
                                      otf.euler.x, otf.euler.y, otf.euler.z) *
                                  ee::math::Quaternion::fromEulerDeg(
                                      occ.euler.x, occ.euler.y, occ.euler.z);
        info = SphereToBox(tf.position + cc.offset, cc.radius,
                           otf.position + occ.offset, occ.halfExtents, qB);
      } else if (cc.shape == ColliderShape::Box &&
                 occ.shape == ColliderShape::Sphere) {
        ee::math::Quaternion qA = ee::math::Quaternion::fromEulerDeg(
                                      tf.euler.x, tf.euler.y, tf.euler.z) *
                                  ee::math::Quaternion::fromEulerDeg(
                                      cc.euler.x, cc.euler.y, cc.euler.z);
        info = SphereToBox(otf.position + occ.offset, occ.radius,
                           tf.position + cc.offset, cc.halfExtents, qA);
        info.normal = -info.normal;
      }
      // ##TODO : box/box, capsule...

      if (!info.hit)
        continue;

      RigideBodyComponent *rbB = nullptr;
      if (_world.hasComponent<RigideBodyComponent>(idB))
        rbB = _world.getComponent<RigideBodyComponent>(idB);

      // ##TODO trigger : si cc.isTrigger || occ.isTrigger -> emettre un
      //        TriggerEvent et NE PAS resoudre (ni position, ni vitesse).

      float wA = rbA ? 1.0f : 0.0f;
      float wB = rbB ? 1.0f : 0.0f;
      float sum = wA + wB;
      if (sum == 0.0f)
        continue;
      wA /= sum;
      wB /= sum;

      tf.position -= info.normal * (info.depth * wA);
      otf.position += info.normal * (info.depth * wB);

      if (rbA) {
        float vn = rbA->velocity.Dot(info.normal);
        if (vn > 0.0f)
          rbA->velocity -= info.normal * vn;
      }
      if (rbB) {
        float vn = rbB->velocity.Dot(info.normal);
        if (vn < 0.0f)
          rbB->velocity -= info.normal * vn;
      }
    }
  }
}
