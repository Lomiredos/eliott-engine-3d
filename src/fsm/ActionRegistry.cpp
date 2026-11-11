#include "visu/fsm/ActionRegistry.hpp"
#include "visu/fsm/State.hpp"
#include "visu/fsm/IdleAction.hpp"
#include "visu/fsm/SetParamAction.hpp"

#include <cstdio>


void ee::fsm::ActionRegistry::reg(const std::string& _name, ActionFactory _factory){
        
    if (m_actionFactorys.count(_name))
        std::fprintf(stderr, "[fsm] action \"%s\" enregistrée deux fois, la seconde remplace la première\n", _name.c_str());
    m_actionFactorys[_name] = std::move(_factory);
}

std::unique_ptr<ee::fsm::Action> ee::fsm::ActionRegistry::create(const std::string& _name) const
{
    auto it = m_actionFactorys.find(_name);
    return it == m_actionFactorys.end() ? nullptr : it->second();
}

void ee::fsm::registerEngineActions(ActionRegistry& _registry){
    _registry.reg("SetParam", makeActionFactory<SetParamAction>());
    _registry.reg("Idle",     makeActionFactory<IdleAction>());
}

ee::fsm::ActionRegistry& ee::fsm::actionRegistry(){
    static ActionRegistry registry = []{
        ActionRegistry r;
        registerEngineActions(r);
        return r;
    }();
    return registry;
}
