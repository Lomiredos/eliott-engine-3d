#pragma once

#include "visu/reflect/FieldVisitor.hpp"
#include "visu/reflect/Reflect.hpp"

#include "math/Vector3.hpp"

struct RigidBodyComponent {
  ee::math::Vector3<float> velocity{0.0f, 0.0f, 0.0f};
  bool isStatic = false;
};

template <> struct ee::reflection::Reflect<RigidBodyComponent> {
  static constexpr const char *name = "RigidBodyComponent";

  static void visit(RigidBodyComponent &_c, ee::reflection::FieldVisitor &_v) {
    _v.visit("Velocity", _c.velocity);
    _v.visit("IsStatic", _c.isStatic);
  }
};