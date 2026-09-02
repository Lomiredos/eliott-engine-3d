#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

#include <string>

// Composant MOTEUR : un mesh charge depuis un .obj (chemin relatif au projet).
struct MeshComponent
{
    std::string path;
};

template <>
struct ee::reflection::Reflect<MeshComponent>
{
    static constexpr const char *name = "MeshComponent";

    static void visit(MeshComponent &_c, ee::reflection::FieldVisitor &_v)
    {
        _v.visit("path", _c.path);
    }
};
