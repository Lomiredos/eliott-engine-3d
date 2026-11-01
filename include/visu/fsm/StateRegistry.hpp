#pragma once
#include <functional>
#include <string>
#include <memory>
#include "visu/fsm/State.hpp"

namespace ee::fsm {


    class StateRegistry{
        
    public:
        void reg(const std::string& _name, std::function<std::unique_ptr<State>()> _factory);
    };

}