#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

// Camera du jeu : posee sur une entite qui a un TransformComponent.
// Position = celle du transform ; elle regarde vers le -Z local de l'entite
// (Euler.x > 0 leve la vue, Euler.y la tourne ; Euler.z / roulis ignore).
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
        _v.visit("Fov", _c.fov);
        _v.visit("NearZ", _c.nearZ);
        _v.visit("FarZ", _c.farZ);
        _v.visit("IsMain", _c.isMain);
    }
};
