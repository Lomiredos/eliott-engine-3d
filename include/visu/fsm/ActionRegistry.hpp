#pragma once
#include <functional>
#include <string>
#include <memory>
#include <unordered_map>


#include "visu/fsm/State.hpp"

namespace ee::fsm {

    using ActionFactory = std::function<std::unique_ptr<Action>()>;
    
    template<typename T>
    ActionFactory makeActionFactory() {return []{return std::make_unique<T>();};}
    
    class ActionRegistry{
    private:
        std::unordered_map<std::string, ActionFactory> m_actionFactorys;
    
    public:
        
    

    void reg(const std::string& _name, ActionFactory _factory);

    std::unique_ptr<ee::fsm::Action> create(const std::string& _name) const;

};

}