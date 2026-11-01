#pragma once


#include <string>
#include <memory>
#include <vector>

#include "visu/reflect/FieldVisitor.hpp"
#include "visu/fsm/Context.hpp"
#include "visu/fsm/Transition.hpp"

namespace ee::fsm {

    class Action
    {
    public:
        virtual ~Action() = default;
        virtual void OnStart(Ctx&){}
        virtual void OnUpdate(Ctx& , float /*_dt*/, float /*_stateTime*/) {}  // no const because it may modifie ctx
        virtual void OnEnd(Ctx&){}
        virtual void VisitParams(ee::reflection::FieldVisitor&) {}
    };

    struct StateDef{
        std::string name;
        std::vector<std::unique_ptr<Action>> actions;
        std::vector<TransitionDef> transition;
    };

} //namespace ee::fsm