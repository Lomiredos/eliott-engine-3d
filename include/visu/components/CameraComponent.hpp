#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

struct CameraComponent
{
    float fov = 45.0f;
    float nearZ = 0.1f;
    float farZ = 200.0f;
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
        _v.separator();
        _v.visit("IsMain", _c.isMain);
    }
};
