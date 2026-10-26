#include "visu/core/SceneInfo.hpp"
#include "visu/core/SceneJson.hpp"

#include <fstream>

nlohmann::json fieldValueToJson(const FieldValue &value) {
  // On visite l'alternative active (bool/int/float/string) et on ecrit sa
  // vraie valeur JSON.
  return std::visit([](const auto &val) { return nlohmann::json(val); }, value);
}

std::optional<FieldValue> fieldValueFromJson(const nlohmann::json &v) {
  if (v.is_boolean())
    return FieldValue(v.get<bool>());
  if (v.is_number_integer())
    return FieldValue(v.get<int>());
  if (v.is_number_float())
    return FieldValue(v.get<float>());
  if (v.is_string())
    return FieldValue(v.get<std::string>());
  return std::nullopt;
}

nlohmann::json sceneToJson(const SceneInfo &scene) {
  nlohmann::json j;
  j["name"] = scene.name;
  j["entities"] = nlohmann::json::array();
  for (const auto &ent : scene.entities) {
    nlohmann::json je;
    je["name"] = ent.name;
    je["components"] = nlohmann::json::array();
    for (const auto &ci : ent.components) {
      nlohmann::json jc;
      jc["name"] = ci.name;
      jc["values"] = nlohmann::json::object();
      for (const auto &kv : ci.values)
        jc["values"][kv.first] = fieldValueToJson(kv.second);
      je["components"].push_back(std::move(jc));
    }
    j["entities"].push_back(std::move(je));
  }
  return j;
}

SceneInfo sceneFromJson(const nlohmann::json &j) {
  SceneInfo scene;
  scene.name = j.value("name", "");
  if (j.contains("entities") == false)
    return scene;

  for (const auto &e : j["entities"]) {
    EntityInfo ent;
    ent.name = e.value("name", "");
    if (e.contains("components") == false) {
      scene.entities.push_back(std::move(ent));
      continue;
    }

    for (const auto &c : e["components"]) {
      ComponentInstance ci;

      if (c.is_object() == false)
        continue;

      ci.name = c.value("name", "");
      if (c.contains("values") && c["values"].is_object())
        for (auto it = c["values"].begin(); it != c["values"].end(); ++it)
          if (std::optional<FieldValue> v = fieldValueFromJson(it.value()))
            ci.values[it.key()] = *v;
      if (!ci.name.empty())
        ent.components.push_back(std::move(ci));
    }
    scene.entities.push_back(std::move(ent));
  }
  return scene;
}

std::optional<SceneInfo> loadScene(const std::filesystem::path &jsonPath) {
  std::ifstream f(jsonPath);
  if (!f)
    return std::nullopt;

  try {
    SceneInfo scene = sceneFromJson(nlohmann::json::parse(f));
    scene.name = jsonPath.stem().string(); // le nom vient du fichier
    return scene;
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

bool saveScene(const SceneInfo &scene, const std::filesystem::path &jsonPath) {
  nlohmann::json j = sceneToJson(scene);
  j.erase("name"); // le nom de la scene est celui du fichier

  std::error_code ec;
  if (jsonPath.has_parent_path())
    std::filesystem::create_directories(jsonPath.parent_path(), ec);

  std::ofstream f(jsonPath);
  if (!f)
    return false;

  f << j.dump(2);
  return true;
}
