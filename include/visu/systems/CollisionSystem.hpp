#pragma once

#include <cmath>
#include <vector>

#include "ecs/System.hpp"
#include "ecs/World.hpp"

#include "math/Quaternion.hpp"

#include "visu/components/ColliderComponent.hpp"
#include "visu/components/RigideBodyComponent.hpp"
#include "visu/components/TransformComponent.hpp"
#include "visu/scene/Hierarchy.hpp"

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

// Box/box entre deux boites orientees (OBB) : test des axes separateurs
// (3 axes de A, 3 de B, 9 produits croises). Le contact garde l'axe de plus
// faible penetration ; la normale va de A vers B, comme les autres tests.
inline Contact BoxToBox(const ee::math::Vector3<float> &_centerA,
                        const ee::math::Vector3<float> &_halfA,
                        const ee::math::Quaternion &_rotA,
                        const ee::math::Vector3<float> &_centerB,
                        const ee::math::Vector3<float> &_halfB,
                        const ee::math::Quaternion &_rotB) {
  const ee::math::Vector3<float> axA[3] = {_rotA.right(), _rotA.up(),
                                           _rotA.back()};
  const ee::math::Vector3<float> axB[3] = {_rotB.right(), _rotB.up(),
                                           _rotB.back()};
  const float hA[3] = {_halfA.x, _halfA.y, _halfA.z};
  const float hB[3] = {_halfB.x, _halfB.y, _halfB.z};
  ee::math::Vector3<float> d = _centerB - _centerA;

  Contact best;
  best.depth = 1e30f;

  // Renvoie false si l'axe separe les deux boites (pas de contact).
  auto testAxis = [&](ee::math::Vector3<float> _axis) {
    float len2 = _axis.Dot(_axis);
    if (len2 < 1e-8f)
      return true; // produit croise d'axes paralleles : axe inutilisable
    _axis = _axis / std::sqrt(len2);

    float rA = 0.0f, rB = 0.0f;
    for (int k = 0; k < 3; ++k) {
      rA += hA[k] * std::fabs(axA[k].Dot(_axis));
      rB += hB[k] * std::fabs(axB[k].Dot(_axis));
    }
    float dist = d.Dot(_axis);
    float overlap = rA + rB - std::fabs(dist);
    if (overlap <= 0.0f)
      return false;
    if (overlap < best.depth) {
      best.depth = overlap;
      best.normal = dist < 0.0f ? -_axis : _axis;
    }
    return true;
  };

  for (int k = 0; k < 3; ++k)
    if (!testAxis(axA[k]) || !testAxis(axB[k]))
      return {};
  for (int a = 0; a < 3; ++a)
    for (int b = 0; b < 3; ++b)
      if (!testAxis(axA[a].Cross(axB[b])))
        return {};

  best.hit = true;
  return best;
}

// Collider d'une entite exprime dans le monde : meme calcul que le rendu
// wire (RenderSystem), pour que la physique colle a ce qu'on voit. L'offset
// est mis a l'echelle puis tourne par le transform ; la rotation du collider
// se compose apres celle du transform ; les dimensions suivent l'echelle.
struct WorldCollider {
  ColliderShape shape = ColliderShape::Sphere;
  ee::math::Vector3<float> center;
  ee::math::Quaternion rot;
  ee::math::Vector3<float> half; // Box
  float radius = 0.0f;           // Sphere
};

// _tf = transform MONDE de l'entite (parent resolu, voir worldXform).
inline WorldCollider toWorld(const ee::scene::Xform &_tf,
                             const ColliderComponent &_cc) {
  WorldCollider w;
  w.shape = _cc.shape;
  const ee::math::Quaternion &qTf = _tf.rot;
  w.rot = qTf * ee::math::Quaternion::fromEulerDeg(_cc.euler.x, _cc.euler.y,
                                                   _cc.euler.z);
  w.center = _tf.pos + qTf.rotate(_cc.offset * _tf.scale);
  w.half = _cc.halfExtents * _tf.scale;
  // Une sphere mise a l'echelle de facon non uniforme est un ellipsoide :
  // on prend le plus grand axe (la sphere englobante).
  float s = std::fmax(std::fabs(_tf.scale.x),
                      std::fmax(std::fabs(_tf.scale.y), std::fabs(_tf.scale.z)));
  w.radius = _cc.radius * s;
  return w;
}

inline Contact collide(const WorldCollider &_a, const WorldCollider &_b) {
  using S = ColliderShape;
  if (_a.shape == S::Sphere && _b.shape == S::Sphere)
    return SphereToSphere(_a.center, _b.center, _a.radius, _b.radius);
  if (_a.shape == S::Sphere && _b.shape == S::Box)
    return SphereToBox(_a.center, _a.radius, _b.center, _b.half, _b.rot);
  if (_a.shape == S::Box && _b.shape == S::Sphere) {
    Contact c = SphereToBox(_b.center, _b.radius, _a.center, _a.half, _a.rot);
    c.normal = -c.normal;
    return c;
  }
  if (_a.shape == S::Box && _b.shape == S::Box)
    return BoxToBox(_a.center, _a.half, _a.rot, _b.center, _b.half, _b.rot);
  // ##TODO : capsule
  return {};
}

class CollisionSystem : public ee::ecs::UpdateSystem {
public:
  void update(ee::ecs::World &_world, float _dt) override;
};

inline void CollisionSystem::update(ee::ecs::World &_world, float _dt) {
  (void)_dt;
  std::vector<ee::ecs::EntityID> ents(m_entities.begin(), m_entities.end());

  for (size_t i = 0; i < ents.size(); ++i) {
    ee::ecs::EntityID idA = ents[i];

    ColliderComponent &cc = *_world.getComponent<ColliderComponent>(idA);

    if (!cc.isActive)
      continue;

    RigideBodyComponent *rbA = nullptr;
    if (_world.hasComponent<RigideBodyComponent>(idA))
      rbA = _world.getComponent<RigideBodyComponent>(idA);

    for (size_t j = i + 1; j < ents.size(); ++j) {
      ee::ecs::EntityID idB = ents[j];

      ColliderComponent &occ = *_world.getComponent<ColliderComponent>(idB);

      if (!occ.isActive)
        continue;

      // Recalcule a chaque paire : une resolution precedente a pu bouger A.
      // Transforms monde : un collider peut etre porte par un enfant.
      Contact info = collide(toWorld(ee::scene::worldXform(_world, idA), cc),
                             toWorld(ee::scene::worldXform(_world, idB), occ));
      if (!info.hit)
        continue;

      RigideBodyComponent *rbB = nullptr;
      if (_world.hasComponent<RigideBodyComponent>(idB))
        rbB = _world.getComponent<RigideBodyComponent>(idB);

      // ##TODO trigger : si cc.isTrigger || occ.isTrigger -> emettre un
      //        TriggerEvent et NE PAS resoudre (ni position, ni vitesse).

      // Un corps statique se comporte comme un decor : il ne bouge pas.
      bool dynA = rbA && !rbA->isStatic;
      bool dynB = rbB && !rbB->isStatic;
      float wA = dynA ? 1.0f : 0.0f;
      float wB = dynB ? 1.0f : 0.0f;
      float sum = wA + wB;
      if (sum == 0.0f)
        continue;
      wA /= sum;
      wB /= sum;

      // Correction exprimee dans le monde, convertie dans le repere du parent.
      if (wA > 0.0f)
        ee::scene::moveWorld(_world, idA, -(info.normal * (info.depth * wA)));
      if (wB > 0.0f)
        ee::scene::moveWorld(_world, idB, info.normal * (info.depth * wB));

      if (dynA) {
        float vn = rbA->velocity.Dot(info.normal);
        if (vn > 0.0f)
          rbA->velocity -= info.normal * vn;
      }
      if (dynB) {
        float vn = rbB->velocity.Dot(info.normal);
        if (vn < 0.0f)
          rbB->velocity -= info.normal * vn;
      }
    }
  }
}
