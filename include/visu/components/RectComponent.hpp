#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

// Composant MOTEUR : une boite (largeur x hauteur x profondeur).
struct RectComponent
{
    float width = 1.0f;
    float height = 1.0f;
    float depth = 1.0f;
};

template <>
struct ee::reflection::Reflect<RectComponent>
{
    static constexpr const char *name = "RectComponent";

    static void visit(RectComponent &_c, ee::reflection::FieldVisitor &_v)
    {
        _v.visit("width", _c.width);
        _v.visit("height", _c.height);
        _v.visit("depth", _c.depth);
    }
};
