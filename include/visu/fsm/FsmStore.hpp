#pragma once
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "visu/fsm/StateMachine.hpp"
#include "visu/fsm/ActionRegistry.hpp"
#include "visu/fsm/ConditionRegistry.hpp"

namespace ee::fsm {

    struct FsmRegistries {
        const ActionRegistry    &actions;
        const ConditionRegistry &conditions;
    };

    std::optional<StateMachineDef> fsmFromJson(
        const nlohmann::json& _root,
        const FsmRegistries& _registries,
        std::vector<std::string> &_errors,
        std::vector<std::string> *_warnings = nullptr);

    std::optional<StateMachineDef> fsmFromJsonText(
        const std::string& _text,
        const FsmRegistries& _registries,
        std::vector<std::string> &_errors,
        std::vector<std::string> *_warnings = nullptr);

    nlohmann::ordered_json fsmToJson(const StateMachineDef& _def);
    std::string fsmToJsonText(const StateMachineDef& _def);

    std::optional<StateMachineDef> loadStateMachineFile(
        const std::filesystem::path& _file,
        const FsmRegistries& _registries,
        std::vector<std::string> &_errors,
        std::vector<std::string> *_warnings = nullptr);
    bool saveStateMachineFile(const std::filesystem::path& _file, const StateMachineDef& _def);

    std::shared_ptr<const StateMachineDef> getStateMachine(const std::string& _assetPath);

}
