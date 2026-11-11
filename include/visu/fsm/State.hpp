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
        virtual void OnUpdate(Ctx&, float /*_dt*/, float /*_stateTime*/) {}
        virtual void OnEnd(Ctx&){}
        virtual void VisitParams(ee::reflection::FieldVisitor&) {}
    };

    struct ActionEntry{
        std::string type;
        std::unique_ptr<Action> action;
        std::string rawJson;
    };

    struct EditorPos{
        float x = 0.f;
        float y = 0.f;
    };

    struct StateDef{
        std::string name;
        std::vector<ActionEntry> actions;
        std::vector<TransitionDef> transition;
        EditorPos editor;
    };

} //namespace ee::fsm