#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"

#include <string>

struct MaterialComponent
{
    std::string path;
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
