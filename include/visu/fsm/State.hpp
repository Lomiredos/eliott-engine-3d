#pragma once


#include <string>
#include <memory>
#include <vector>

#include "visu/reflect/FieldVisitor.hpp"
#include "visu/fsm/Context.hpp"
#include "visu/fsm/Transition.hpp"

namespace ee::fsm {

    class State
    {
    public:
        virtual ~State() = default;
        virtual void OnStart(Ctx& _owner) = 0;
        virtual void OnUpdate(Ctx& _owner, float _dt) = 0; // no const because it may modifie owner
        virtual void OnEnd(Ctx& _owner) = 0;
        virtual void VisitParams(ee::reflection::FieldVisitor&) {}
    };

    struct StateDef{
        std::string name;
        std::unique_ptr<State> state;
        std::vector<TransitionDef> transition;
    };

} //namespace ee::fsm