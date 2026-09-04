#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

#include "math/Vector3.hpp"

enum class ColliderShape : int
{
    Sphere,
    Box,
    Capsule,
    Count
};

template <>
struct EnumNames<ColliderShape>
{
    static constexpr const char *values[] = {"Sphere", "Box", "Capsule"};
    static constexpr int count = 3;
};

struct ColliderComponent
{

    ColliderShape shape = ColliderShape::Sphere;

    ee::math::Vector3<float> offset{0.0f, 0.0f, 0.0f};
    ee::math::Vector3<float> euler{0.0f, 0.0f, 0.0f};

    bool isActive = true;
    bool isTrigger = false;

    // specific

    float radius = 1.0f;
    ee::math::Vector3<float> halfExtents{0.5f, 0.5f, 0.5f};
    float height = 2.0f;
};

template <>
struct ee::reflection::Reflect<ColliderComponent>
{
    static constexpr const char *name = "ColliderComponent";

    static void visit(ColliderComponent &_c, ee::reflection::FieldVisitor &_v)
    {

        _v.visit("Shape", _c.shape);
        _v.visit("OffSet", _c.offset);
        _v.visit("Euler", _c.euler);
        _v.visit("IsActive", _c.isActive);
        _v.visit("IsTrigger", _c.isTrigger);
        _v.visit("radius", _c.radius);
        _v.visit("HalfExtents", _c.halfExtents);
        _v.visit("Height", _c.height);
    }
};