#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

#include "math/Vector3.hpp"

struct RectComponent
{

    ee::math::Vector3<float> size{1.0f, 1.0f, 1.0f};
};

template <>
struct ee::reflection::Reflect<RectComponent>
{
    static constexpr const char *name = "RectComponent";

    static void visit(RectComponent &_c, ee::reflection::FieldVisitor &_v)
    {
        _v.visit("size", _c.size);
    }
};
