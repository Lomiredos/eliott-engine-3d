#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

#include "math/Quaternion.hpp"
#include "math/Vector3.hpp"

#include "visu/core/EntityRef.hpp"

// Avec un parent, position/euler/scale sont LOCAUX (relatifs au parent) ;
// le transform monde se calcule avec ee::scene::worldXform (Hierarchy.hpp).
struct TransformComponent
{
    ee::math::Vector3<float> position{0.0f, 0.0f, 0.0f};
    ee::math::Vector3<float> euler{0.0f, 0.0f, 0.0f};
    ee::math::Vector3<float> scale{1.0f, 1.0f, 1.0f};
    ee::EntityRefOf<TransformComponent> parent; // aucun = racine

    // Axes unitaires tires de euler (convention du moteur : forward = -Z).
    // Comme euler, ils sont LOCAUX : pour une entite avec parent, prendre
    // ee::scene::worldXform(world, e).rot.forward() / up() / right().
    ee::math::Quaternion getRotation() const
    {
        return ee::math::Quaternion::fromEulerDeg(euler.x, euler.y, euler.z);
    }
    ee::math::Vector3<float> getForward() const { return getRotation().forward(); }
    ee::math::Vector3<float> getUp() const { return getRotation().up(); }
    ee::math::Vector3<float> getRight() const { return getRotation().right(); }
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
        _v.visit("Parent", _c.parent);
    }
};
