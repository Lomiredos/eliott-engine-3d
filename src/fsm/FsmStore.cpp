#include "visu/fsm/FsmStore.hpp"
#include "visu/fsm/ActionRegistry.hpp"
#include "visu/fsm/Condition.hpp"
#include "visu/fsm/ConditionRegistry.hpp"
#include "visu/fsm/Operation.hpp"
#include "visu/fsm/Params.hpp"
#include "visu/fsm/State.hpp"
#include "visu/fsm/StateMachine.hpp"
#include "visu/fsm/Transition.hpp"
#include "visu/fsm/Utils.hpp"
#include "visu/reflect/JsonReadVisitor.hpp"
#include "visu/reflect/JsonWriteVisitor.hpp"
#include "visu/core/MeshStore.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>


namespace {

    constexpr int kSupportedVersion = 2;
    const std::string kAnyState = "Any";
    const std::string kStateTime = "$stateTime";

    struct Log {
        std::vector<std::string>& errors;
        std::vector<std::string>& warnings;
    };

    std::string getString(const nlohmann::json& _j, const char* _key){
        auto it = _j.find(_key);
        if (it == _j.end() || !it->is_string()) return "";
        return it->get<std::string>();
    }

    std::optional<ee::fsm::ParamType> parseType(const std::string& _string){
        if (_string == "bool")         return ee::fsm::ParamType::Bool;
        else if (_string == "trigger") return ee::fsm::ParamType::Trigger;
        else if (_string == "float")   return ee::fsm::ParamType::Float;
        else if (_string == "int")     return ee::fsm::ParamType::Int;
        else                           return std::nullopt;
    }

    std::optional<ee::fsm::Op> parseOp(const std::string& _string){
        if (_string == "==")      return ee::fsm::Op::Eq;
        else if (_string == "!=") return ee::fsm::Op::Neq;
        else if (_string == "<")  return ee::fsm::Op::Lt;
        else if (_string == ">")  return ee::fsm::Op::Gt;
        else if (_string == "<=") return ee::fsm::Op::Le;
        else if (_string == ">=") return ee::fsm::Op::Ge;
        else                      return std::nullopt;
    }

    bool isOrderingOp(ee::fsm::Op _op){
        return _op == ee::fsm::Op::Lt || _op == ee::fsm::Op::Gt
            || _op == ee::fsm::Op::Le || _op == ee::fsm::Op::Ge;
    }

    std::optional<ee::fsm::ParamValue> parseValue(const nlohmann::json& _j, ee::fsm::ParamType _type){
        switch (_type) {
            case ee::fsm::ParamType::Bool:
            case ee::fsm::ParamType::Trigger:
                if (_j.is_boolean()) return ee::fsm::ParamValue(_j.get<bool>());
                break;
            case ee::fsm::ParamType::Int:
                if (_j.is_number_integer()) return ee::fsm::ParamValue(_j.get<int>());
                break;
            case ee::fsm::ParamType::Float:
                if (_j.is_number()) return ee::fsm::ParamValue(_j.get<float>());
                break;
        }
        return std::nullopt;
    }

    ee::fsm::ParamValue zeroValue(ee::fsm::ParamType _type){
        switch (_type) {
            case ee::fsm::ParamType::Int:   return ee::fsm::ParamValue(0);
            case ee::fsm::ParamType::Float: return ee::fsm::ParamValue(0.f);
            case ee::fsm::ParamType::Bool:
            case ee::fsm::ParamType::Trigger: break;
        }
        return ee::fsm::ParamValue(false);
    }

    std::vector<ee::fsm::ParamDef> parseParams(const nlohmann::json& _params, Log& _log){
        std::vector<ee::fsm::ParamDef> params;
        for (size_t i = 0; i < _params.size(); i++){
            const std::string where = "parameters[" + std::to_string(i) + "]";

            if (!_params[i].is_object()){
                _log.errors.push_back(where + ": must be an object");
                continue;
            }

            std::string name = getString(_params[i], "name");
            if (name.empty()){
                _log.errors.push_back(where + ": \"name\" not found or invalid");
                continue;
            }
            if (name[0] == '$'){
                _log.errors.push_back(where + ": \"" + name + "\" cannot start with '$' (reserved)");
                continue;
            }

            bool duplicate = false;
            for (const ee::fsm::ParamDef& p : params)
                if (p.name == name) duplicate = true;
            if (duplicate){
                _log.errors.push_back(where + ": parameter \"" + name + "\" is defined twice");
                continue;
            }

            std::string typeName = getString(_params[i], "type");
            std::optional<ee::fsm::ParamType> type = parseType(typeName);
            if (!type.has_value()){
                _log.errors.push_back(where + ": unknown type \"" + typeName + "\" on parameter " + name);
                continue;
            }

            ee::fsm::ParamValue value = zeroValue(type.value());
            if (type.value() != ee::fsm::ParamType::Trigger && _params[i].contains("default")){
                std::optional<ee::fsm::ParamValue> parsed = parseValue(_params[i]["default"], type.value());
                if (!parsed.has_value()){
                    _log.errors.push_back(where + ": default value of " + name + " does not match type " + typeName);
                    continue;
                }
                value = parsed.value();
            }

            ee::fsm::ParamDef pdef;
            pdef.name = name;
            pdef.type = type.value();
            pdef.defaultValue = value;
            params.push_back(pdef);
        }
        return params;
    }

    std::vector<ee::fsm::ActionEntry> parseActions(
        const nlohmann::json& _actions,
        const ee::fsm::ActionRegistry& _register,
        Log& _log,
        const std::string& _stateName)
    {
        std::vector<ee::fsm::ActionEntry> actions;

        for (size_t i = 0; i < _actions.size(); i++){
            const std::string where = "state " + _stateName + ": actions[" + std::to_string(i) + "]";

            if (!_actions[i].is_object()){
                _log.errors.push_back(where + ": must be an object");
                continue;
            }

            std::string type = getString(_actions[i], "type");
            if (type.empty()){
                _log.errors.push_back(where + ": no \"type\" field");
                continue;
            }

            ee::fsm::ActionEntry entry;
            entry.type = type;
            entry.action = _register.create(type);
            if (entry.action == nullptr){
                _log.warnings.push_back(where + ": action type \"" + type + "\" isn't registered, ignored");
                entry.rawJson = _actions[i].dump();
            }
            else {
                ee::reflection::JsonReadVisitor reader(&_actions[i]);
                entry.action->VisitParams(reader);
            }

            actions.push_back(std::move(entry));
        }
        return actions;
    }

    std::vector<ee::fsm::StateDef> parseStates(const nlohmann::json& _states, const ee::fsm::ActionRegistry& _register, Log& _log){
        std::vector<ee::fsm::StateDef> states;

        for (size_t i = 0; i < _states.size(); i++){
            const std::string where = "states[" + std::to_string(i) + "]";

            if (!_states[i].is_object()){
                _log.errors.push_back(where + ": must be an object");
                continue;
            }

            std::string name = getString(_states[i], "name");
            if (name.empty()){
                _log.errors.push_back(where + ": \"name\" not found or invalid");
                continue;
            }
            if (name == kAnyState){
                _log.errors.push_back(where + ": a state cannot be named \"" + kAnyState + "\" (reserved)");
                continue;
            }

            bool duplicate = false;
            for (const ee::fsm::StateDef& s : states)
                if (s.name == name) duplicate = true;
            if (duplicate){
                _log.errors.push_back(where + ": state \"" + name + "\" is defined twice");
                continue;
            }

            if (!_states[i].contains("actions") || !_states[i]["actions"].is_array()){
                _log.errors.push_back(where + ": state " + name + " must contain an \"actions\" array, even if it's empty");
                continue;
            }

            ee::fsm::StateDef sdef;
            sdef.name = name;
            sdef.actions = parseActions(_states[i]["actions"], _register, _log, name);

            auto editor = _states[i].find("editor");
            if (editor != _states[i].end() && editor->is_object()){
                if (editor->contains("x") && (*editor)["x"].is_number()) sdef.editor.x = (*editor)["x"].get<float>();
                if (editor->contains("y") && (*editor)["y"].is_number()) sdef.editor.y = (*editor)["y"].get<float>();
            }
            states.push_back(std::move(sdef));
        }
        return states;
    }
    
    int findState(const ee::fsm::StateMachineDef& _stateMachineDef, const std::string& _stateName){
        if (_stateName == kAnyState) return -2;

        for (size_t i = 0; i < _stateMachineDef.states.size(); i++){
            if (_stateMachineDef.states[i].name == _stateName)
                return static_cast<int>(i);
        }
        return -1;
    }

    std::optional<ee::fsm::ConditionDef> parseCondition(
        const nlohmann::json& _cond,
        const ee::fsm::StateMachineDef& _stateMachineDef,
        const ee::fsm::ConditionRegistry& _register,
        Log& _log,
        const std::string& _where)
    {
        if (!_cond.is_object()){
            _log.errors.push_back(_where + ": must be an object");
            return std::nullopt;
        }

        if (_cond.contains("custom")){
            std::string name = getString(_cond, "custom");
            if (name.empty()){
                _log.errors.push_back(_where + ": \"custom\" must be a non-empty string");
                return std::nullopt;
            }

            bool expected = true;
            if (_cond.contains("expected")){
                if (!_cond["expected"].is_boolean()){
                    _log.errors.push_back(_where + ": \"expected\" must be a bool");
                    return std::nullopt;
                }
                expected = _cond["expected"].get<bool>();
            }

            ee::fsm::ConditionFn fn = _register.find(name);
            if (fn == nullptr)
                _log.warnings.push_back(_where + ": custom condition \"" + name + "\" isn't registered, always false");

            return ee::fsm::CustomCondition{fn, expected, name};
        }

        std::string param = getString(_cond, "param");
        if (param.empty()){
            _log.errors.push_back(_where + ": no \"param\" (or \"custom\") field");
            return std::nullopt;
        }

        std::optional<ee::fsm::Op> op;
        if (_cond.contains("op")){
            std::string opName = getString(_cond, "op");
            op = parseOp(opName);
            if (!op.has_value()){
                _log.errors.push_back(_where + ": unknown op \"" + opName + "\"");
                return std::nullopt;
            }
        }

        if (param == kStateTime){
            if (!op.has_value()){
                _log.errors.push_back(_where + ": " + kStateTime + " needs an \"op\"");
                return std::nullopt;
            }
            if (!_cond.contains("value") || !_cond["value"].is_number()){
                _log.errors.push_back(_where + ": " + kStateTime + " needs a number \"value\"");
                return std::nullopt;
            }
            return ee::fsm::StateTimeCondition{op.value(), _cond["value"].get<float>()};
        }

        if (param[0] == '$'){
            _log.errors.push_back(_where + ": unknown built-in value \"" + param + "\"");
            return std::nullopt;
        }

        int index = ee::fsm::findParam(_stateMachineDef, param);
        if (index < 0){
            _log.errors.push_back(_where + ": unknown parameter \"" + param + "\"");
            return std::nullopt;
        }
        ee::fsm::ParamType type = _stateMachineDef.params[index].type;
        bool isBoolLike = type == ee::fsm::ParamType::Bool || type == ee::fsm::ParamType::Trigger;

        if (isBoolLike){
            if (op.has_value() && isOrderingOp(op.value())){
                _log.errors.push_back(_where + ": op <, >, <=, >= not allowed on bool/trigger \"" + param + "\"");
                return std::nullopt;
            }
        }
        else if (!op.has_value()){
            _log.errors.push_back(_where + ": \"op\" is required on int/float parameter \"" + param + "\"");
            return std::nullopt;
        }

        ee::fsm::ParamValue value(true);
        if (_cond.contains("value")){
            std::optional<ee::fsm::ParamValue> parsed = parseValue(_cond["value"], type);
            if (!parsed.has_value()){
                _log.errors.push_back(_where + ": \"value\" does not match the type of parameter \"" + param + "\"");
                return std::nullopt;
            }
            value = parsed.value();
        }
        else if (!isBoolLike){
            _log.errors.push_back(_where + ": \"value\" is required on int/float parameter \"" + param + "\"");
            return std::nullopt;
        }

        return ee::fsm::ParamCondition{index, op.value_or(ee::fsm::Op::Eq), value};
    }

    std::vector<ee::fsm::ConditionDef> parseConditions(
        const nlohmann::json& _conditions,
        const ee::fsm::StateMachineDef& _stateMachineDef,
        const ee::fsm::ConditionRegistry& _register,
        Log& _log,
        const std::string& _where)
    {
        std::vector<ee::fsm::ConditionDef> conditions;

        for (size_t i = 0; i < _conditions.size(); i++){
            std::optional<ee::fsm::ConditionDef> cond = parseCondition(
                _conditions[i], _stateMachineDef, _register, _log,
                _where + ": conditions[" + std::to_string(i) + "]");
            if (cond.has_value())
                conditions.push_back(cond.value());
        }
        return conditions;
    }

    void parseTransitions(
        ee::fsm::StateMachineDef& _stateMachineDef,
        const nlohmann::json& _transitions,
        const ee::fsm::ConditionRegistry& _register,
        Log& _log)
    {
        for (size_t i = 0; i < _transitions.size(); i++){
            const std::string where = "transitions[" + std::to_string(i) + "]";
            const nlohmann::json& t = _transitions[i];

            if (!t.is_object()){
                _log.errors.push_back(where + ": must be an object");
                continue;
            }

            std::string from = getString(t, "from");
            if (from.empty()){
                _log.errors.push_back(where + ": must have a \"from\" state");
                continue;
            }

            std::string to = getString(t, "to");
            if (to.empty()){
                _log.errors.push_back(where + ": must have a \"to\" state");
                continue;
            }

            int fromIndex = findState(_stateMachineDef, from);
            int toIndex = findState(_stateMachineDef, to);

            if (fromIndex == -1){
                _log.errors.push_back(where + ": unknown state \"" + from + "\"");
                continue;
            }
            if (toIndex == -1){
                _log.errors.push_back(where + ": unknown state \"" + to + "\"");
                continue;
            }
            if (toIndex == -2){
                _log.errors.push_back(where + ": a transition cannot go to " + kAnyState);
                continue;
            }

            bool allowSelf = false;
            if (t.contains("allowSelf")){
                if (!t["allowSelf"].is_boolean()){
                    _log.errors.push_back(where + ": \"allowSelf\" must be a bool");
                    continue;
                }
                allowSelf = t["allowSelf"].get<bool>();
                if (allowSelf && fromIndex != -2){
                    _log.errors.push_back(where + ": \"allowSelf\" only makes sense with \"from\": \"" + kAnyState + "\"");
                    continue;
                }
            }

            ee::fsm::TransitionDef tDef;
            if (t.contains("conditions")){
                if (!t["conditions"].is_array()){
                    _log.errors.push_back(where + ": \"conditions\" must be an array");
                    continue;
                }
                tDef.conditions = parseConditions(t["conditions"], _stateMachineDef, _register, _log, where);
            }
            tDef.targetState = toIndex;
            tDef.allowSelf = allowSelf;

            if (fromIndex == -2)
                _stateMachineDef.anyTransition.push_back(std::move(tDef));
            else
                _stateMachineDef.states[fromIndex].transition.push_back(std::move(tDef));
        }
    }
}


std::optional<ee::fsm::StateMachineDef> ee::fsm::fsmFromJson(
    const nlohmann::json &_root,
    const ee::fsm::FsmRegistries &_registries,
    std::vector<std::string> &_errors,
    std::vector<std::string> *_warnings)
{
    std::vector<std::string> warnings;
    Log log{_errors, _warnings ? *_warnings : warnings};
    const std::size_t errorCount = _errors.size();

    if (!_root.is_object()){
        _errors.push_back("root must be a json object");
        return std::nullopt;
    }

    if (!_root.contains("version") || !_root["version"].is_number_integer()){
        _errors.push_back("\"version\" must be written and must be an int");
    }
    else if (_root["version"].get<int>() > kSupportedVersion){
        _errors.push_back("version " + std::to_string(_root["version"].get<int>())
                         + " is newer than the supported version " + std::to_string(kSupportedVersion));
    }

    if (!_root.contains("states") || !_root["states"].is_array() || _root["states"].empty()){
        _errors.push_back("\"states\" must be written and must be a non-empty array");
        return std::nullopt;
    }

    ee::fsm::StateMachineDef def;
    def.name = getString(_root, "name");

    if (_root.contains("parameters")){
        if (_root["parameters"].is_array())
            def.params = parseParams(_root["parameters"], log);
        else
            _errors.push_back("\"parameters\" must be an array");
    }

    def.states = parseStates(_root["states"], _registries.actions, log);

    std::string entryState = getString(_root, "entryState");
    if (entryState.empty()){
        _errors.push_back("\"entryState\" must be written");
    }
    else {
        int entryIndex = findState(def, entryState);
        if (entryIndex < 0)
            _errors.push_back("entryState: unknown state \"" + entryState + "\"");
        else
            def.entryState = entryIndex;
    }

    if (!_root.contains("transitions") || !_root["transitions"].is_array()){
        _errors.push_back("\"transitions\" must be written and must be an array, even if empty");
    }
    else {
        parseTransitions(def, _root["transitions"], _registries.conditions, log);
    }

    if (_errors.size() != errorCount) return std::nullopt;
    return def;
}


std::optional<ee::fsm::StateMachineDef> ee::fsm::fsmFromJsonText(
    const std::string &_text,
    const ee::fsm::FsmRegistries &_registries,
    std::vector<std::string> &_errors,
    std::vector<std::string> *_warnings)
{
    nlohmann::json root = nlohmann::json::parse(_text, nullptr, false);
    if (root.is_discarded()){
        _errors.push_back("invalid JSON (syntax error)");
        return std::nullopt;
    }
    try {
        return fsmFromJson(root, _registries, _errors, _warnings);
    }
    catch (const std::exception &e){
        _errors.push_back(std::string("unexpected error while reading: ") + e.what());
        return std::nullopt;
    }
}


namespace {

    const char* typeToString(ee::fsm::ParamType _type){
        switch (_type) {
            case ee::fsm::ParamType::Bool:    return "bool";
            case ee::fsm::ParamType::Trigger: return "trigger";
            case ee::fsm::ParamType::Float:   return "float";
            case ee::fsm::ParamType::Int:     return "int";
        }
        return "bool";
    }

    const char* opToString(ee::fsm::Op _op){
        switch (_op) {
            case ee::fsm::Op::Eq:  return "==";
            case ee::fsm::Op::Neq: return "!=";
            case ee::fsm::Op::Lt:  return "<";
            case ee::fsm::Op::Gt:  return ">";
            case ee::fsm::Op::Le:  return "<=";
            case ee::fsm::Op::Ge:  return ">=";
        }
        return "==";
    }

    nlohmann::ordered_json valueToJson(const ee::fsm::ParamValue& _value){
        if (const float* f = std::get_if<float>(&_value)) return ee::reflection::shortestFloat(*f);
        if (const int*   i = std::get_if<int>(&_value))   return *i;
        return std::get<bool>(_value);
    }

    nlohmann::ordered_json actionToJson(const ee::fsm::ActionEntry& _entry){
        if (_entry.action == nullptr){
            nlohmann::ordered_json raw = nlohmann::ordered_json::parse(_entry.rawJson, nullptr, false);
            if (raw.is_object()) return raw;
            return nlohmann::ordered_json{{"type", _entry.type}};
        }
        nlohmann::ordered_json j = nlohmann::ordered_json::object();
        j["type"] = _entry.type;
        ee::reflection::JsonWriteVisitor writer(&j);
        _entry.action->VisitParams(writer);
        j["type"] = _entry.type;
        return j;
    }

    nlohmann::ordered_json conditionToJson(const ee::fsm::ConditionDef& _cond, const ee::fsm::StateMachineDef& _def){
        nlohmann::ordered_json j = nlohmann::ordered_json::object();

        if (const ee::fsm::ParamCondition* c = std::get_if<ee::fsm::ParamCondition>(&_cond)){
            if (c->param < 0 || c->param >= (int)_def.params.size()) return j;
            const ee::fsm::ParamDef& p = _def.params[c->param];
            j["param"] = p.name;

            bool isBoolLike = p.type == ee::fsm::ParamType::Bool || p.type == ee::fsm::ParamType::Trigger;
            const bool* b = std::get_if<bool>(&c->value);
            if (isBoolLike && c->op == ee::fsm::Op::Eq && b && *b)
                return j;

            j["op"] = opToString(c->op);
            j["value"] = valueToJson(c->value);
        }
        else if (const ee::fsm::StateTimeCondition* c = std::get_if<ee::fsm::StateTimeCondition>(&_cond)){
            j["param"] = kStateTime;
            j["op"] = opToString(c->op);
            j["value"] = ee::reflection::shortestFloat(c->value);
        }
        else if (const ee::fsm::CustomCondition* c = std::get_if<ee::fsm::CustomCondition>(&_cond)){
            j["custom"] = c->name;
            if (!c->expected) j["expected"] = false;
        }
        return j;
    }

    nlohmann::ordered_json transitionToJson(const ee::fsm::TransitionDef& _t, const std::string& _from, const ee::fsm::StateMachineDef& _def){
        nlohmann::ordered_json j = nlohmann::ordered_json::object();
        j["from"] = _from;
        if (_t.targetState >= 0 && _t.targetState < (int)_def.states.size())
            j["to"] = _def.states[_t.targetState].name;
        if (_t.allowSelf) j["allowSelf"] = true;

        nlohmann::ordered_json conds = nlohmann::ordered_json::array();
        for (const ee::fsm::ConditionDef& c : _t.conditions)
            conds.push_back(conditionToJson(c, _def));
        j["conditions"] = conds;
        return j;
    }
}

nlohmann::ordered_json ee::fsm::fsmToJson(const StateMachineDef &_def)
{
    nlohmann::ordered_json root = nlohmann::ordered_json::object();
    root["version"] = kSupportedVersion;
    if (!_def.name.empty()) root["name"] = _def.name;

    nlohmann::ordered_json params = nlohmann::ordered_json::array();
    for (const ParamDef& p : _def.params){
        nlohmann::ordered_json jp = nlohmann::ordered_json::object();
        jp["name"] = p.name;
        jp["type"] = typeToString(p.type);
        if (p.type != ParamType::Trigger) jp["default"] = valueToJson(p.defaultValue);
        params.push_back(jp);
    }
    root["parameters"] = params;

    nlohmann::ordered_json states = nlohmann::ordered_json::array();
    for (const StateDef& s : _def.states){
        nlohmann::ordered_json js = nlohmann::ordered_json::object();
        js["name"] = s.name;
        nlohmann::ordered_json actions = nlohmann::ordered_json::array();
        for (const ActionEntry& a : s.actions)
            actions.push_back(actionToJson(a));
        js["actions"] = actions;
        js["editor"] = { {"x", ee::reflection::shortestFloat(s.editor.x)},
                         {"y", ee::reflection::shortestFloat(s.editor.y)} };
        states.push_back(js);
    }
    root["states"] = states;

    if (_def.entryState >= 0 && _def.entryState < (int)_def.states.size())
        root["entryState"] = _def.states[_def.entryState].name;

    nlohmann::ordered_json transitions = nlohmann::ordered_json::array();
    for (const TransitionDef& t : _def.anyTransition)
        transitions.push_back(transitionToJson(t, kAnyState, _def));
    for (const StateDef& s : _def.states)
        for (const TransitionDef& t : s.transition)
            transitions.push_back(transitionToJson(t, s.name, _def));
    root["transitions"] = transitions;

    return root;
}

std::string ee::fsm::fsmToJsonText(const StateMachineDef &_def)
{
    return fsmToJson(_def).dump(4) + "\n";
}


std::optional<ee::fsm::StateMachineDef> ee::fsm::loadStateMachineFile(
    const std::filesystem::path &_file,
    const FsmRegistries &_registries,
    std::vector<std::string> &_errors,
    std::vector<std::string> *_warnings)
{
    const std::string prefix = _file.string() + ": ";

    std::ifstream f(_file);
    if (!f){
        _errors.push_back(prefix + "cannot open file");
        return std::nullopt;
    }
    std::stringstream ss;
    ss << f.rdbuf();

    std::vector<std::string> errors, warnings;
    std::optional<StateMachineDef> def = fsmFromJsonText(ss.str(), _registries, errors, &warnings);

    for (const std::string& e : errors) _errors.push_back(prefix + e);
    if (_warnings)
        for (const std::string& w : warnings) _warnings->push_back(prefix + w);
    return def;
}

bool ee::fsm::saveStateMachineFile(const std::filesystem::path &_file, const StateMachineDef &_def)
{
    std::error_code ec;
    if (_file.has_parent_path())
        std::filesystem::create_directories(_file.parent_path(), ec);
    std::ofstream f(_file, std::ios::binary);
    if (!f)
        return false;
    f << fsmToJsonText(_def);
    return f.good();
}


namespace {
    struct CachedFsm {
        std::shared_ptr<const ee::fsm::StateMachineDef> def;
        std::filesystem::file_time_type mtime{};
        std::chrono::steady_clock::time_point lastCheck{};
        bool checked = false;
        bool missing = false;
    };
    std::unordered_map<std::string, CachedFsm> g_fsmCache;
}

std::shared_ptr<const ee::fsm::StateMachineDef> ee::fsm::getStateMachine(const std::string &_assetPath)
{
    if (_assetPath.empty())
        return nullptr;
    const std::filesystem::path full = ee::core::assetPath(_assetPath);
    CachedFsm &c = g_fsmCache[full.string()];

    auto now = std::chrono::steady_clock::now();
    if (c.checked && now - c.lastCheck < std::chrono::seconds(1))
        return c.def;
    c.lastCheck = now;

    std::error_code ec;
    auto mtime = std::filesystem::last_write_time(full, ec);
    if (ec){
        if (!c.missing)
            std::fprintf(stderr, "[fsm] %s: file not found\n", full.string().c_str());
        c.missing = true;
        c.checked = true;
        c.def.reset();
        return nullptr;
    }

    if (!c.checked || c.missing || mtime != c.mtime){
        const bool reload = c.def != nullptr;
        c.mtime = mtime;
        c.missing = false;

        std::vector<std::string> errors, warnings;
        std::optional<StateMachineDef> def = loadStateMachineFile(
            full, {actionRegistry(), conditionRegistry()}, errors, &warnings);

        for (const std::string& w : warnings)
            std::fprintf(stderr, "[fsm] warning: %s\n", w.c_str());

        if (def.has_value()){
            c.def = std::make_shared<const StateMachineDef>(std::move(*def));
            if (reload)
                std::fprintf(stderr, "[fsm] %s: reloaded\n", full.string().c_str());
        }
        else {
            for (const std::string& e : errors)
                std::fprintf(stderr, "[fsm] error: %s\n", e.c_str());
            if (reload)
                std::fprintf(stderr, "[fsm] %s: invalid, keeping the previous version\n", full.string().c_str());
        }
    }
    c.checked = true;
    return c.def;
}
