#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

// Composant MOTEUR : une sphere de rayon donne.
struct SphereComponent
{
    float radius = 1.0f;
};

template <>
struct ee::reflection::Reflect<SphereComponent>
{
    static constexpr const char *name = "SphereComponent";

    static void visit(SphereComponent &_c, ee::reflection::FieldVisitor &_v)
    {
        _v.visit("radius", _c.radius);
    }
};
