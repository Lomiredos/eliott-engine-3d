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
        _v.visit("Position", _c.position);
        _v.visit("Euleur", _c.euler);
        _v.visit("Scale", _c.scale);
    }
};
