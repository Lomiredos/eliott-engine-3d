#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

#include "math/Vector3.hpp"


struct RigideBodyComponent {
    ee::math::Vector3<float> velocity{0.0f, 0.0f, 0.0f};
};


template <>
struct ee::reflection::Reflect<RigideBodyComponent> 
{
    static constexpr const char* name = "RigideBodyComponent";

    static void visit(RigideBodyComponent& _c, ee::reflection::FieldVisitor& _v)
    {
        _v.visit("VelocityX", _c.velocity.x);
        _v.visit("VelocityY", _c.velocity.y);
        _v.visit("VelocityZ", _c.velocity.z);
    }
};