#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

// Camera du jeu : posee sur une entite qui a un TransformComponent.
// Position = celle du transform ; elle regarde vers le -Z local de l'entite
// (EuleurX > 0 leve la vue, EuleurY la tourne ; EuleurZ / roulis ignore).
// Plusieurs cameras : la premiere avec isMain gagne, sinon la premiere tout
// court. Aucune : camera par defaut du runtime.
struct CameraComponent
{
    float fov = 45.0f; // champ de vision vertical, degres
    float nearZ = 0.1f;  // distance min affichee, > 0 (borne a 0.01)
    float farZ = 200.0f; // distance max affichee, > nearZ
    bool isMain = true;
};

template <>
struct ee::reflection::Reflect<CameraComponent>
{
    static constexpr const char *name = "CameraComponent";

    static void visit(CameraComponent &_c, ee::reflection::FieldVisitor &_v)
    {
        _v.visit("fov", _c.fov);
        _v.visit("nearZ", _c.nearZ);
        _v.visit("farZ", _c.farZ);
        _v.visit("isMain", _c.isMain);
    }
};
