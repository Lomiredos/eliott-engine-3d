#pragma once

#include "math/Vector3.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace ee::core {
struct CpuMesh {
  std::vector<float> verts;
  std::vector<unsigned int> idx;
  ee::math::Vector3<float> aabbMin;
  ee::math::Vector3<float> aabbMax;
  unsigned int gpu = 0;
  bool valid = false;
};

void setMeshBaseDir(const std::filesystem::path &_dir);

CpuMesh &getMesh(const std::string &_path);
} // namespace ee::core
