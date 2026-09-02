#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

#include "math/Vector3.hpp"

struct TransformComponent
{
    ee::math::Vector3<float> position{0.0f, 0.0f, 0.0f};
    ee::math::Vector3<float> euler{0.0f, 0.0f, 0.0f};
    ee::math::Vector3<float> scale{1.0f, 1.0f, 1.0f};
};

template <>
struct ee::reflection::Reflect<TransformComponent>
{
    static constexpr const char *name = "TransformComponent";

    static void visit(TransformComponent &_c, ee::reflection::FieldVisitor &_v)
    {
        _v.visit("x", _c.position.x);
        _v.visit("y", _c.position.y);
        _v.visit("z", _c.position.z);
        _v.visit("rotX", _c.euler.x);
        _v.visit("rotY", _c.euler.y);
        _v.visit("rotZ", _c.euler.z);
        _v.visit("scaleX", _c.scale.x);
        _v.visit("scaleY", _c.scale.y);
        _v.visit("scaleZ", _c.scale.z);
    }
};
