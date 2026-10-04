#include "visu/core/SceneInfo.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

std::optional<SceneInfo> loadScene(const std::filesystem::path &jsonPath) {
  std::ifstream f(jsonPath);
  if (!f)
    return std::nullopt;

  try {
    nlohmann::json j = nlohmann::json::parse(f);

    SceneInfo scene;
    scene.name = jsonPath.stem().string();
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
          for (auto it = c["values"].begin(); it != c["values"].end(); ++it) {
            const auto &v = it.value();
            if (v.is_boolean())
              ci.values[it.key()] = v.get<bool>();
            else if (v.is_number_integer())
              ci.values[it.key()] = v.get<int>();
            else if (v.is_number_float())
              ci.values[it.key()] = v.get<float>();
            else if (v.is_string())
              ci.values[it.key()] = v.get<std::string>();
          }
        if (!ci.name.empty())
          ent.components.push_back(std::move(ci));
      }
      scene.entities.push_back(std::move(ent));
    }
    return scene;
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

bool saveScene(const SceneInfo &scene, const std::filesystem::path &jsonPath) {
  nlohmann::json j;
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
        // kv.second est un variant : on visite l'alternative active
        // (bool/int/float/string) et on ecrit sa vraie valeur JSON.
        std::visit([&](const auto &val) { jc["values"][kv.first] = val; },
                   kv.second);
      je["components"].push_back(std::move(jc));
    }
    j["entities"].push_back(std::move(je));
  }

  std::error_code ec;
  if (jsonPath.has_parent_path())
    std::filesystem::create_directories(jsonPath.parent_path(), ec);

  std::ofstream f(jsonPath);
  if (!f)
    return false;

  f << j.dump(2);
  return true;
}
