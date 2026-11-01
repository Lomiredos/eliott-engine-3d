#include "visu/fsm/API.hpp"


#include <unordered_set>
#include <cstdio>

#include "visu/fsm/Utils.hpp"

namespace {
    using namespace ee::fsm;

    std::unordered_set<std::string> invalide_arg;


    void reportOnce(const std::string& _key, const std::string& _msg){
        if (invalide_arg.insert(_key).second){
            std::fprintf(stderr, "[fsm] %s\n", _msg.c_str());
        }
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

