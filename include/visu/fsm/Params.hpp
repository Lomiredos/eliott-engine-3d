#pragma once
#include <variant>
#include <string>

#include "visu/reflect/FieldVisitor.hpp"


namespace ee::fsm{

    enum class ParamType {Bool, Float, Int, Trigger};
    using ParamValue = std::variant<bool, float, int>;
    
    struct ParamDef{
        std::string name;
        ParamType type;
        ParamValue defaultValue;
    };
}


template <>
struct EnumNames<ee::fsm::ParamValue>
{
    static constexpr const char *values[] = {"bool", "float", "int"};
    static constexpr int count = 3;
};
