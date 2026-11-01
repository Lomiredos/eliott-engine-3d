#include "visu/fsm/StateRegistry.hpp"


void ee::fsm::StateRegistry::reg(const std::string& _name, std::function<std::unique_ptr<State>()> _factory){
    
}