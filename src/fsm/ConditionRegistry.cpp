#include "visu/fsm/ConditionRegistry.hpp"

#include <cstdio>

void ee::fsm::ConditionRegistry::reg(const std::string &_name, ConditionFn _cfn)
{
    if (m_functions.count(_name))
        std::fprintf(stderr, "[fsm] condition \"%s\" enregistrée deux fois, la seconde remplace la première\n", _name.c_str());
    m_functions[_name] = _cfn;
}
ee::fsm::ConditionFn ee::fsm::ConditionRegistry::find(const std::string &_name) const
{
    auto it = m_functions.find(_name);
    return it == m_functions.end() ? nullptr : it->second;
}

ee::fsm::ConditionRegistry& ee::fsm::conditionRegistry(){
    static ConditionRegistry registry;
    return registry;
}
