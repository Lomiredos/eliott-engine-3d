#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

using FieldValue = std::variant<float, int, bool, std::string>;

struct ComponentInstance {
  std::string name;
  std::map<std::string, FieldValue> values;
};

struct EntityInfo {
  std::string name;
  std::vector<ComponentInstance> components;
};

struct SceneInfo {
  std::string name;
  std::vector<EntityInfo> entities;
};
std::optional<SceneInfo> loadScene(const std::filesystem::path &jsonPath);

bool saveScene(const SceneInfo &scene, const std::filesystem::path &jsonPath);
