#pragma once
#include <variant>
#include <string>



namespace ee::fsm{

    enum class ParamType {Bool, Float, Int, Trigger};
    using ParamValue = std::variant<bool, float, int>;
    
    struct ParamDef{
        std::string name;
        ParamType type;
        ParamValue defaultValue;
    };
}