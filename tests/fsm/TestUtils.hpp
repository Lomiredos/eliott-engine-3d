#pragma once

#include <doctest/doctest.h>

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

#include "ecs/World.hpp"
#include "visu/fsm/API.hpp"
#include "visu/fsm/FsmStore.hpp"
#include "visu/fsm/Utils.hpp"

namespace fsmtest {

    inline std::vector<std::string>& journal(){
        static std::vector<std::string> j;
        return j;
    }

    class RecordingAction : public ee::fsm::Action {
        static std::string stateName(const ee::fsm::Ctx& _ctx){
            return _ctx.def.states[_ctx.inst.currentState].name;
        }
    public:
        void OnStart(ee::fsm::Ctx& _ctx) override { journal().push_back(stateName(_ctx) + ":enter"); }
        void OnEnd(ee::fsm::Ctx& _ctx) override   { journal().push_back(stateName(_ctx) + ":exit"); }
    };

    inline bool g_grounded = true;
    inline bool isGrounded(const ee::fsm::Ctx&){ return g_grounded; }

    struct Registries {
        ee::fsm::ActionRegistry actions;
        ee::fsm::ConditionRegistry conditions;

        Registries(){
            ee::fsm::registerEngineActions(actions);
            actions.reg("Rec", ee::fsm::makeActionFactory<RecordingAction>());
            actions.reg("PlayPose", ee::fsm::makeActionFactory<RecordingAction>());
            conditions.reg("IsGrounded", &isGrounded);
        }
        ee::fsm::FsmRegistries get() const { return {actions, conditions}; }
    };

    inline const Registries& registries(){
        static Registries r;
        return r;
    }

    struct Loaded {
        std::optional<ee::fsm::StateMachineDef> def;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    inline Loaded load(const std::string& _text){
        Loaded l;
        l.def = ee::fsm::fsmFromJsonText(_text, registries().get(), l.errors, &l.warnings);
        return l;
    }

    inline bool hasMessage(const std::vector<std::string>& _msgs, std::initializer_list<const char*> _parts){
        return std::any_of(_msgs.begin(), _msgs.end(), [&](const std::string& m){
            for (const char* p : _parts)
                if (m.find(p) == std::string::npos) return false;
            return true;
        });
    }

    inline std::string recStates(std::initializer_list<const char*> _names){
        std::string s = "[";
        bool first = true;
        for (const char* n : _names){
            if (!first) s += ",";
            s += std::string(R"({"name":")") + n + R"(","actions":[{"type":"Rec"}]})";
            first = false;
        }
        return s + "]";
    }

    inline std::string makeFsm(const std::string& _params, const std::string& _states,
                               const std::string& _entry, const std::string& _transitions){
        return R"({"version":2,"name":"T","parameters":)" + _params
             + R"(,"states":)" + _states
             + R"(,"entryState":")" + _entry
             + R"(","transitions":)" + _transitions + "}";
    }

    inline ee::fsm::StateMachineDef mustLoad(const std::string& _text){
        Loaded l = load(_text);
        INFO("errors: ", l.errors.size(), " first: ", l.errors.empty() ? "" : l.errors[0]);
        REQUIRE(l.def.has_value());
        return std::move(*l.def);
    }

    struct Runner {
        ee::ecs::World world;
        const ee::fsm::StateMachineDef& def;
        ee::fsm::StateMachineInstance inst;

        explicit Runner(const ee::fsm::StateMachineDef& _def) : def(_def) {
            ee::fsm::initInstance(def, inst);
        }

        void tick(float _dt = 0.1f){
            ee::fsm::Ctx ctx{world, 0, def, inst};
            ee::fsm::update(def, ctx, inst, _dt);
        }

        void ticks(int _n, float _dt = 0.1f){
            for (int i = 0; i < _n; i++) tick(_dt);
        }

        std::string state() const {
            if (inst.currentState < 0 || inst.currentState >= (int)def.states.size()) return "";
            return def.states[inst.currentState].name;
        }

        int tickUntil(const std::string& _state, int _max = 50, float _dt = 0.1f){
            for (int i = 1; i <= _max; i++){
                tick(_dt);
                if (state() == _state) return i;
            }
            return -1;
        }
    };

}
