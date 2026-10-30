#pragma once

#include <cmath>
#include <vector>

#include "ecs/System.hpp"
#include "ecs/World.hpp"

#include "math/Quaternion.hpp"

#include "visu/core/EventBus.hpp"

#include "visu/components/ColliderComponent.hpp"
#include "visu/components/RigideBodyComponent.hpp"
#include "visu/components/TransformComponent.hpp"
#include "visu/scene/Hierarchy.hpp"

struct Contact {
  bool hit = false;
  ee::math::Vector3<float> normal{0.0f, 0.0f, 0.0f};
  float depth = 0.0f;
};

// Publie sur l'EventBus quand un collider trigger chevauche un autre collider
// (a chaque frame tant que le chevauchement dure). Aucune resolution : ni
// position ni vitesse ne sont touchees. triggerEntity porte le collider
// trigger (si les deux le sont, c'est l'un des deux, l'autre est
// triggeredEntity). penetration = normale * profondeur, orientee de
// triggerEntity vers triggeredEntity.
struct TriggerEvent {
  ee::ecs::EntityID triggerEntity;
  ee::ecs::EntityID triggeredEntity;
  ee::math::Vector3<float> penetration;
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

// ---------------------------------------------------------------------------
// Capsule : segment [_p0, _p1] (axe Y local du collider, longueur = height)
// arrondi d'un rayon _r. Tous les tests se ramenent a des spheres placees au
// point le plus proche du segment.
// ---------------------------------------------------------------------------

inline ee::math::Vector3<float>
ClosestOnSegment(const ee::math::Vector3<float> &_p,
                 const ee::math::Vector3<float> &_a,
                 const ee::math::Vector3<float> &_b) {
  ee::math::Vector3<float> ab = _b - _a;
  float len2 = ab.Dot(ab);
  if (len2 < 1e-12f)
    return _a;
  float t = (_p - _a).Dot(ab) / len2;
  t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
  return _a + ab * t;
}

// Points les plus proches entre les segments [_p1,_q1] et [_p2,_q2]
// (Ericson, Real-Time Collision Detection, 5.1.9).
inline void ClosestSegmentSegment(const ee::math::Vector3<float> &_p1,
                                  const ee::math::Vector3<float> &_q1,
                                  const ee::math::Vector3<float> &_p2,
                                  const ee::math::Vector3<float> &_q2,
                                  ee::math::Vector3<float> &_c1,
                                  ee::math::Vector3<float> &_c2) {
  auto clamp01 = [](float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); };
  const float eps = 1e-12f;
  ee::math::Vector3<float> d1 = _q1 - _p1;
  ee::math::Vector3<float> d2 = _q2 - _p2;
  ee::math::Vector3<float> r = _p1 - _p2;
  float a = d1.Dot(d1);
  float e = d2.Dot(d2);
  float f = d2.Dot(r);
  float s, t;

  if (a <= eps && e <= eps) {
    s = t = 0.0f;
  } else if (a <= eps) {
    s = 0.0f;
    t = clamp01(f / e);
  } else {
    float c = d1.Dot(r);
    if (e <= eps) {
      t = 0.0f;
      s = clamp01(-c / a);
    } else {
      float b = d1.Dot(d2);
      float denom = a * e - b * b;
      s = denom > eps ? clamp01((b * f - c * e) / denom) : 0.0f;
      t = (b * s + f) / e;
      if (t < 0.0f) {
        t = 0.0f;
        s = clamp01(-c / a);
      } else if (t > 1.0f) {
        t = 1.0f;
        s = clamp01((b - c) / a);
      }
    }
  }
  _c1 = _p1 + d1 * s;
  _c2 = _p2 + d2 * t;
}

inline Contact CapsuleToSphere(const ee::math::Vector3<float> &_p0,
                               const ee::math::Vector3<float> &_p1, float _rA,
                               const ee::math::Vector3<float> &_center,
                               float _rB) {
  return SphereToSphere(ClosestOnSegment(_center, _p0, _p1), _center, _rA,
                        _rB);
}

inline Contact CapsuleToCapsule(const ee::math::Vector3<float> &_a0,
                                const ee::math::Vector3<float> &_a1, float _rA,
                                const ee::math::Vector3<float> &_b0,
                                const ee::math::Vector3<float> &_b1,
                                float _rB) {
  ee::math::Vector3<float> cA, cB;
  ClosestSegmentSegment(_a0, _a1, _b0, _b1, cA, cB);
  return SphereToSphere(cA, cB, _rA, _rB);
}

// Capsule/box : la distance d'un point du segment a la boite est convexe le
// long du segment, on cherche son minimum par recherche ternaire puis on fait
// un test sphere/box en ce point. Si le segment traverse la boite, on garde
// le point (min trouve ou extremites) qui penetre le plus.
inline Contact CapsuleToBox(const ee::math::Vector3<float> &_p0,
                            const ee::math::Vector3<float> &_p1, float _r,
                            const ee::math::Vector3<float> &_boxCenter,
                            const ee::math::Vector3<float> &_half,
                            const ee::math::Quaternion &_boxRot) {
  ee::math::Vector3<float> seg = _p1 - _p0;

  auto dist2ToBox = [&](float _t) {
    ee::math::Vector3<float> d = (_p0 + seg * _t) - _boxCenter;
    float lx = d.Dot(_boxRot.right()), ly = d.Dot(_boxRot.up()),
          lz = d.Dot(_boxRot.back());
    auto out = [](float v, float h) {
      float o = std::fabs(v) - h;
      return o > 0.0f ? o : 0.0f;
    };
    float ox = out(lx, _half.x), oy = out(ly, _half.y), oz = out(lz, _half.z);
    return ox * ox + oy * oy + oz * oz;
  };

  float lo = 0.0f, hi = 1.0f;
  for (int it = 0; it < 32; ++it) {
    float m1 = lo + (hi - lo) / 3.0f;
    float m2 = hi - (hi - lo) / 3.0f;
    if (dist2ToBox(m1) <= dist2ToBox(m2))
      hi = m2;
    else
      lo = m1;
  }
  float tBest = 0.5f * (lo + hi);

  Contact best = SphereToBox(_p0 + seg * tBest, _r, _boxCenter, _half, _boxRot);
  if (best.hit && dist2ToBox(tBest) <= 1e-8f) {
    for (float t : {0.0f, 1.0f}) {
      Contact c = SphereToBox(_p0 + seg * t, _r, _boxCenter, _half, _boxRot);
      if (c.hit && c.depth > best.depth)
        best = c;
    }
  }
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
  float radius = 0.0f;           // Sphere, Capsule
  ee::math::Vector3<float> p0;   // Capsule : extremites du segment
  ee::math::Vector3<float> p1;
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
  if (w.shape == ColliderShape::Capsule) {
    // Comme le wire : rayon suit l'echelle X, longueur du segment l'echelle Y.
    w.radius = _cc.radius * std::fabs(_tf.scale.x);
    ee::math::Vector3<float> axis =
        w.rot.up() * (0.5f * _cc.height * std::fabs(_tf.scale.y));
    w.p0 = w.center - axis;
    w.p1 = w.center + axis;
  }
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
  if (_a.shape == S::Capsule && _b.shape == S::Capsule)
    return CapsuleToCapsule(_a.p0, _a.p1, _a.radius, _b.p0, _b.p1, _b.radius);
  if (_a.shape == S::Capsule && _b.shape == S::Sphere)
    return CapsuleToSphere(_a.p0, _a.p1, _a.radius, _b.center, _b.radius);
  if (_a.shape == S::Capsule && _b.shape == S::Box)
    return CapsuleToBox(_a.p0, _a.p1, _a.radius, _b.center, _b.half, _b.rot);
  // B capsule, A sphere/box : test inverse, normale retournee (A -> B).
  if (_b.shape == S::Capsule) {
    Contact c = collide(_b, _a);
    c.normal = -c.normal;
    return c;
  }
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

      // Trigger : on previent, on ne resout pas (ni position, ni vitesse).
      if (cc.isTrigger || occ.isTrigger) {
        ee::math::Vector3<float> pen = info.normal * info.depth; // A -> B
        if (cc.isTrigger)
          ee::core::EventBus::getInstance().publish(TriggerEvent{idA, idB, pen});
        else
          ee::core::EventBus::getInstance().publish(TriggerEvent{idB, idA, -pen});
        continue;
      }

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
