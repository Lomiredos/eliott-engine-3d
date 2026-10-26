#pragma once

#include "visu/core/SceneInfo.hpp"

#include <nlohmann/json.hpp>

#include <optional>

// Conversion SceneInfo <-> JSON, partagee par les fichiers de scene et le
// lien editeur <-> jeu (snapshots, modifications de champs).

nlohmann::json fieldValueToJson(const FieldValue &value);
// nullopt si le JSON n'est ni bool, ni nombre, ni chaine.
std::optional<FieldValue> fieldValueFromJson(const nlohmann::json &j);

// {"name": ..., "entities": [{"name", "components": [{"name", "values"}]}]}
nlohmann::json sceneToJson(const SceneInfo &scene);
SceneInfo sceneFromJson(const nlohmann::json &j);
