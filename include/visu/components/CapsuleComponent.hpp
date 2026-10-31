#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

struct CapsuleComponent
{
    float radius = 0.5f;
    float height = 1.0f;
};

template <>
struct ee::reflection::Reflect<CapsuleComponent>
{
    static constexpr const char *name = "CapsuleComponent";

    static void visit(CapsuleComponent &_c, ee::reflection::FieldVisitor &_v)
    {
        _v.visit("Radius", _c.radius);
        _v.visit("Height", _c.height);
    }
};
