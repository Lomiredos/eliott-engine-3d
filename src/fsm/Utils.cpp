#include "visu/fsm/Utils.hpp"

#include <cassert>

#include "visu/utils/Overloaded.hpp"



namespace {

    using namespace ee::fsm;

    template <typename V>
    bool compare(const V &_a, Op op, const V &_b){
        switch (op)  {
            case Op::Eq:  return _a == _b;
            case Op::Neq: return _a != _b;
            case Op::Lt:  return _a <  _b;
            case Op::Gt:  return _a >  _b;
            case Op::Le:  return _a <= _b;
            case Op::Ge:  return _a >= _b;
        }
        return false;
    }

    bool evalCondition(const ConditionDef &_cond, const Ctx &_ctx, const StateMachineInstance &_inst){
        return std::visit(ee::overloaded{
            
            [&](const ParamCondition &_c) -> bool {
                if (_c.param >= (int)_inst.params.size()) return false;
                const ParamValue &current = _inst.params[_c.param];
                return std::visit([&](const auto &a) -> bool {
                    using V = std::decay_t<decltype(a)>;
                    const V* b = std::get_if<V>(&_c.value);
                    return b && compare(a, _c.op, *b);
                }, current);
            },
            [&](const StateTimeCondition &_c) -> bool {
                return compare(_inst.stateTime, _c.op, _c.value);
            },
            [&](const CustomCondition &_c) {
                return _c.fn && _c.fn(_ctx) == _c.expected;
            },
            
            
        }, _cond);
    }
    
    bool isValid(const TransitionDef &_td, const Ctx &_ctx, const StateMachineInstance &_inst){
        
        for (const ConditionDef &cd : _td.conditions){
            if (evalCondition(cd, _ctx, _inst)) continue;
            return false;
        }
        return true;
    }

    void resetTrigger(const StateMachineDef &_def, const TransitionDef& _td, StateMachineInstance &_inst){
        for (const ConditionDef &cd : _td.conditions){

            const ParamCondition* pc = std::get_if<ParamCondition>(&cd);

            if (!pc || pc->param >= (int)_def.params.size())
                continue;
            if (_def.params[pc->param].type == ParamType::Trigger){
                if (pc->param >= (int)_inst.params.size()) continue;
                _inst.params[pc->param] = false;
            }
        }
    }

    void changeState(const StateMachineDef &_def, Ctx &_ctx, StateMachineInstance &_inst, const TransitionDef& _td){
        assert(_td.targetState >= 0 && _td.targetState < (int)_def.states.size() && "targetState invalid");
        if (_td.targetState < 0 || _td.targetState >= (int)_def.states.size()) return; 

        for (auto& a : _def.states[_inst.currentState].actions)
            a->OnEnd(_ctx);
    
        resetTrigger(_def, _td, _inst);
        _inst.currentState = _td.targetState;
        _inst.stateTime = 0.f;
    
        for (auto& a : _def.states[_inst.currentState].actions)
            a->OnStart(_ctx);
}


}








int ee::fsm::findParam(const StateMachineDef &_def, const std::string &_name) {
    for (std::size_t i = 0; i < _def.params.size(); i++){
        if (_def.params[i].name == _name)
            return (int)i;
    }
    return -1;
}


    
void ee::fsm::update(const StateMachineDef &_def, Ctx &_ctx, StateMachineInstance &_inst, float _dt){

    //init
    if (_inst.currentState == -1){
        
        assert(_def.entryState >= 0 && _def.entryState < (int)_def.states.size() && "targetState invalid");
        if (_def.entryState < 0 || _def.entryState >= (int)_def.states.size()) return; 

        _inst.currentState = _def.entryState;
        for (auto& a : _def.states[_inst.currentState].actions)
            a->OnStart(_ctx);
        _inst.stateTime = 0.f;
        return;
    }

    bool transited = false;
    //check des transition Any
    for (const TransitionDef &td : _def.anyTransition){
        if (td.allowSelf == false)
            if (td.targetState == _inst.currentState) continue;

        if (isValid(td, _ctx, _inst)){
            changeState(_def, _ctx, _inst, td);
            transited = true;
            break;
        }
    }

    
    //check des transition si Any n'a pas eu lieu
    if (transited == false){
        for (const TransitionDef& td : _def.states[_inst.currentState].transition){
            if (isValid(td, _ctx, _inst))
            {
                changeState(_def, _ctx, _inst, td);
                break;
            }
        }
    }
    //update 
    for (auto& a : _def.states[_inst.currentState].actions)
        a->OnUpdate(_ctx, _dt, _inst.stateTime);
    _inst.stateTime += _dt;

}

void ee::fsm::initInstance(const StateMachineDef &_def, StateMachineInstance &_inst){
    _inst.params.clear();
    for (const ParamDef &pd : _def.params){
        _inst.params.push_back(pd.defaultValue);
    }
    _inst.currentState = -1;
    _inst.stateTime = 0.f;
    _inst.initializedWith = &_def;
}
