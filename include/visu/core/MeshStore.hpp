#pragma once

#include "math/Vector3.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace ee::core {
struct CpuMesh {
  std::vector<float> verts; // format ee::render::kVertexFloats (MeshUtil.hpp)
  std::vector<unsigned int> idx;
  ee::math::Vector3<float> aabbMin;
  ee::math::Vector3<float> aabbMax;
  unsigned int gpu = 0;
  bool valid = false;
};

// Dossier racine des assets (racine du projet) : les chemins relatifs des
// meshes, materiaux et textures sont resolus depuis lui.
void setMeshBaseDir(const std::filesystem::path &_dir);

// Chemin d'asset (relatif a la racine du projet, ou absolu) -> chemin complet.
std::filesystem::path assetPath(const std::string &_path);

CpuMesh &getMesh(const std::string &_path);
} // namespace ee::core
