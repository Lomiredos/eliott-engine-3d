#pragma once
#include <unordered_map>
#include <string>

#include "visu/fsm/Condition.hpp"


namespace ee::fsm{
    class ConditionRegistry{
        std::unordered_map<std::string, ConditionFn> m_functions;
        
    public:
        void reg(const std::string& _name, ConditionFn _cfn);
        ConditionFn find(const std::string &_name) const;

    };

    ConditionRegistry& conditionRegistry();
}