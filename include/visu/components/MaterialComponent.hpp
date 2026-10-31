#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

#include <string>

// Materiau (.mat, voir MaterialStore.hpp) des formes de l'entite (Sphere,
// Rect, Capsule, Mesh). Sans lui : couleur par defaut de la forme.
struct MaterialComponent
{
    std::string path; // ex. "Assets/Materials/Brique.mat"
};

template <>
struct ee::reflection::Reflect<MaterialComponent>
{
    static constexpr const char *name = "MaterialComponent";

    static void visit(MaterialComponent &_c, ee::reflection::FieldVisitor &_v)
    {
        _v.visit("Path", _c.path);
    }
};
