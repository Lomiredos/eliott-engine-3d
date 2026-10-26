#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

// ---------------------------------------------------------------------------
// Lumieres, posees sur des entites avec un TransformComponent.
// Sans aucune lumiere dans la scene : un soleil et une ambiance par defaut.
// ---------------------------------------------------------------------------

// Soleil : eclaire tout, dans la direction du -Z local de l'entite (comme
// une camera : EuleurX negatif = vers le bas). La position ne compte pas.
struct DirectionalLightComponent
{
    ee::Color3 color{1.0f, 1.0f, 1.0f};
    float intensity = 2.5f;
};

// Ampoule : eclaire autour de sa position, jusqu'a range.
struct PointLightComponent
{
    ee::Color3 color{1.0f, 1.0f, 1.0f};
    float intensity = 5.0f;
    float range = 10.0f;
};

// Lumiere ambiante : eclairage minimal partout (une seule utilisee).
struct AmbientLightComponent
{
    ee::Color3 color{1.0f, 1.0f, 1.0f};
    float intensity = 0.25f;
};

template <>
struct ee::reflection::Reflect<DirectionalLightComponent>
{
    static constexpr const char *name = "DirectionalLightComponent";
    static void visit(DirectionalLightComponent &_c, ee::reflection::FieldVisitor &_v)
    {
        _v.visit("color", _c.color);
        _v.visit("intensity", _c.intensity);
    }
};

template <>
struct ee::reflection::Reflect<PointLightComponent>
{
    static constexpr const char *name = "PointLightComponent";
    static void visit(PointLightComponent &_c, ee::reflection::FieldVisitor &_v)
    {
        _v.visit("color", _c.color);
        _v.visit("intensity", _c.intensity);
        _v.visit("range", _c.range);
    }
};

template <>
struct ee::reflection::Reflect<AmbientLightComponent>
{
    static constexpr const char *name = "AmbientLightComponent";
    static void visit(AmbientLightComponent &_c, ee::reflection::FieldVisitor &_v)
    {
        _v.visit("color", _c.color);
        _v.visit("intensity", _c.intensity);
    }
};
