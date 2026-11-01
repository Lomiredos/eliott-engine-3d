#include "visu/fsm/StateMachineDef.hpp"
#include "visu/utils/Overloaded.hpp"
#include <cstddef>
#include <cstdio>
#include <netinet/tcp.h>
#include <type_traits>
#include <variant>
#include <cassert>
#include <string>
#include <unordered_set>

namespace {


    std::unordered_set<std::string> invalide_arg;


    void reportOnce(const std::string& _key, const std::string& _msg){
        if (invalide_arg.insert(_key).second){
            std::fprintf(stderr, "[fsm] %s\n", _msg.c_str());
        }
    }

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

        _def.states[_inst.currentState].state->OnEnd(_ctx);
        resetTrigger(_def, _td, _inst);
        _inst.currentState = _td.targetState;
        _inst.stateTime = 0.f;
        _def.states[_inst.currentState].state->OnStart(_ctx);
    }

    const char* typeName(ParamType _type){
        switch (_type) {
            case ParamType::Bool:    return "Bool";
            case ParamType::Float:   return "Float";
            case ParamType::Int:     return "Int";
            case ParamType::Trigger: return "Trigger";
        }
        return "?";
    }
    
    bool checkParam(const StateMachineDef &_def, const StateMachineInstance &_inst, int _index, ParamType _type, const char* _fn){
        const std::string fn = std::string(_fn) + typeName(_type);

        if (_index < 0 || _index >= (int)_def.params.size()){
            reportOnce(fn + ":badIndex:" + std::to_string(_index),
                       fn + " : index invalide " + std::to_string(_index));
            return false;
        }

        const ParamDef &pd = _def.params[_index];
        if (pd.type != _type){
            reportOnce(fn + ":type:" + pd.name,
                       fn + "(\"" + pd.name + "\") : le param est de type " + typeName(pd.type) + ", appel ignore");
            return false;
        }

        if (_index >= (int)_inst.params.size()){
            reportOnce("uninit", "StateMachineInstance non initialisee (appeler initInstance)");
            return false;
        }
        return true;
    }

    int findParamOrReport(const StateMachineDef &_def, const std::string &_name, ParamType _type, const char* _fn){
        int i = findParam(_def, _name);
        if (i < 0){
            const std::string fn = std::string(_fn) + typeName(_type);
            reportOnce(fn + ":unknown:" + _name, fn + " : param inconnu \"" + _name + "\"");
        }
        return i;
    }

    template<typename T>
    void setParam(const StateMachineDef &_def, StateMachineInstance &_inst, int _index, ParamType _type, T _value){
        if (!checkParam(_def, _inst, _index, _type, "set"))
            return;
        _inst.params[_index] = _value;
    }

    template<typename T>
    void setParam(const StateMachineDef &_def, StateMachineInstance &_inst, const std::string &_name, ParamType _type, T _value){
        int i = findParamOrReport(_def, _name, _type, "set");
        if (i < 0)
            return;
        setParam<T>(_def, _inst, i, _type, _value);
    }

    template<typename T>
    T getParam(const StateMachineDef &_def, const StateMachineInstance &_inst, int _index, ParamType _type){
        if (!checkParam(_def, _inst, _index, _type, "get"))
            return T{};
        const T* v = std::get_if<T>(&_inst.params[_index]);
        return v ? *v : T{};
    }

    template<typename T>
    T getParam(const StateMachineDef &_def, const StateMachineInstance &_inst, const std::string &_name, ParamType _type){
        int i = findParamOrReport(_def, _name, _type, "get");
        if (i < 0)
            return T{};
        return getParam<T>(_def, _inst, i, _type);
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
        _def.states[_inst.currentState].state->OnStart(_ctx);
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
    _def.states[_inst.currentState].state->OnUpdate(_ctx, _dt);
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


void ee::fsm::setBool(const StateMachineDef &_def, StateMachineInstance &_inst, const std::string &_name, bool _value){
    setParam<bool>(_def, _inst, _name, ParamType::Bool, _value);
}

void ee::fsm::setFloat(const StateMachineDef &_def, StateMachineInstance &_inst, const std::string &_name, float _value){
    setParam<float>(_def, _inst, _name, ParamType::Float, _value);
}

void ee::fsm::setInt(const StateMachineDef &_def, StateMachineInstance &_inst, const std::string &_name, int _value){
    setParam<int>(_def, _inst, _name, ParamType::Int, _value);
}

void ee::fsm::setTrigger(const StateMachineDef &_def, StateMachineInstance &_inst, const std::string &_name){
    setParam<bool>(_def, _inst, _name, ParamType::Trigger, true);
}

void ee::fsm::resetTrigger(const StateMachineDef &_def, StateMachineInstance &_inst, const std::string &_name){
    setParam<bool>(_def, _inst, _name, ParamType::Trigger, false);
}

void ee::fsm::setBool(const StateMachineDef &_def, StateMachineInstance &_inst, int _index, bool _value){
    setParam<bool>(_def, _inst, _index, ParamType::Bool, _value);
}

void ee::fsm::setFloat(const StateMachineDef &_def, StateMachineInstance &_inst, int _index, float _value){
    setParam<float>(_def, _inst, _index, ParamType::Float, _value);
}

void ee::fsm::setInt(const StateMachineDef &_def, StateMachineInstance &_inst, int _index, int _value){
    setParam<int>(_def, _inst, _index, ParamType::Int, _value);
}

void ee::fsm::setTrigger(const StateMachineDef &_def, StateMachineInstance &_inst, int _index){
    setParam<bool>(_def, _inst, _index, ParamType::Trigger, true);
}

void ee::fsm::resetTrigger(const StateMachineDef &_def, StateMachineInstance &_inst, int _index){
    setParam<bool>(_def, _inst, _index, ParamType::Trigger, false);
}

bool ee::fsm::getBool(const StateMachineDef &_def, const StateMachineInstance &_inst, const std::string &_name){
    return getParam<bool>(_def, _inst, _name, ParamType::Bool);
}

float ee::fsm::getFloat(const StateMachineDef &_def, const StateMachineInstance &_inst, const std::string &_name){
    return getParam<float>(_def, _inst, _name, ParamType::Float);
}

int ee::fsm::getInt(const StateMachineDef &_def, const StateMachineInstance &_inst, const std::string &_name){
    return getParam<int>(_def, _inst, _name, ParamType::Int);
}

bool ee::fsm::getTrigger(const StateMachineDef &_def, const StateMachineInstance &_inst, const std::string &_name){
    return getParam<bool>(_def, _inst, _name, ParamType::Trigger);
}

bool ee::fsm::getBool(const StateMachineDef &_def, const StateMachineInstance &_inst, int _index){
    return getParam<bool>(_def, _inst, _index, ParamType::Bool);
}

float ee::fsm::getFloat(const StateMachineDef &_def, const StateMachineInstance &_inst, int _index){
    return getParam<float>(_def, _inst, _index, ParamType::Float);
}

int ee::fsm::getInt(const StateMachineDef &_def, const StateMachineInstance &_inst, int _index){
    return getParam<int>(_def, _inst, _index, ParamType::Int);
}

bool ee::fsm::getTrigger(const StateMachineDef &_def, const StateMachineInstance &_inst, int _index){
    return getParam<bool>(_def, _inst, _index, ParamType::Trigger);
}
