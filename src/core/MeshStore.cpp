#include "visu/core/MeshStore.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace {
std::filesystem::path g_baseDir;
}

namespace {
using Vec3 = ee::math::Vector3<float>;

void parseFaceToken(const std::string &_tok, int _posCount, int _normCount,
                    int &_pi, int &_ni) {
  _pi = _ni = -1;
  auto slash1 = _tok.find('/');
  std::string pPart =
      (slash1 == std::string::npos) ? _tok : _tok.substr(0, slash1);
  if (!pPart.empty()) {
    int v = std::stoi(pPart);
    _pi = (v > 0) ? v - 1 : _posCount + v;
  }
  if (slash1 != std::string::npos) {
    auto slash2 = _tok.find('/', slash1 + 1);
    if (slash2 != std::string::npos) {
      std::string nPart = _tok.substr(slash2 + 1);
      if (!nPart.empty()) {
        int v = std::stoi(nPart);
        _ni = (v > 0) ? v - 1 : _normCount + v;
      }
    }
  }
}

ee::core::CpuMesh loadObj(const std::string &_path) {
  ee::core::CpuMesh mesh;
  std::ifstream f(_path);
  if (!f)
    return mesh; // valid == false

  std::vector<Vec3> positions;
  std::vector<Vec3> normals;

  try {
    std::string line;
    while (std::getline(f, line)) {
      std::istringstream ls(line);
      std::string tag;
      ls >> tag;
      if (tag == "v") {
        Vec3 p;
        ls >> p.x >> p.y >> p.z;
        positions.push_back(p);
      } else if (tag == "vn") {
        Vec3 n;
        ls >> n.x >> n.y >> n.z;
        normals.push_back(n);
      } else if (tag == "f") {
        std::vector<std::string> toks;
        std::string t;
        while (ls >> t)
          toks.push_back(t);
        if (toks.size() < 3)
          continue;

        // Triangulation en eventail (v0, vi, vi+1).
        for (size_t i = 1; i + 1 < toks.size(); ++i) {
          const std::string *tri[3] = {&toks[0], &toks[i], &toks[i + 1]};
          int pi[3], ni[3];
          Vec3 pos[3];
          bool ok = true;
          for (int k = 0; k < 3; ++k) {
            parseFaceToken(*tri[k], (int)positions.size(), (int)normals.size(),
                           pi[k], ni[k]);
            if (pi[k] < 0 || pi[k] >= (int)positions.size()) {
              ok = false;
              break;
            }
            pos[k] = positions[pi[k]];
          }
          if (!ok)
            continue;

          // Normale : celle du .obj si dispo, sinon normale de face.
          Vec3 faceN = (pos[1] - pos[0]).Cross(pos[2] - pos[0]).Normalize();
          for (int k = 0; k < 3; ++k) {
            Vec3 n = (ni[k] >= 0 && ni[k] < (int)normals.size())
                         ? normals[ni[k]]
                         : faceN;
            mesh.verts.insert(mesh.verts.end(),
                              {pos[k].x, pos[k].y, pos[k].z, n.x, n.y, n.z});
            mesh.idx.push_back((unsigned int)mesh.idx.size());
          }
        }
      }
    }
  } catch (const std::exception &) {
    return ee::core::CpuMesh{}; // fichier malforme
  }

  if (mesh.idx.empty())
    return mesh; // valid == false

  // AABB depuis toutes les positions (borne valide, meme si super-ensemble).
  mesh.aabbMin = positions[0];
  mesh.aabbMax = positions[0];
  for (const Vec3 &p : positions) {
    mesh.aabbMin.x = std::min(mesh.aabbMin.x, p.x);
    mesh.aabbMin.y = std::min(mesh.aabbMin.y, p.y);
    mesh.aabbMin.z = std::min(mesh.aabbMin.z, p.z);
    mesh.aabbMax.x = std::max(mesh.aabbMax.x, p.x);
    mesh.aabbMax.y = std::max(mesh.aabbMax.y, p.y);
    mesh.aabbMax.z = std::max(mesh.aabbMax.z, p.z);
  }

  mesh.valid = true;
  return mesh;
}
} // namespace

namespace ee::core {
void setMeshBaseDir(const std::filesystem::path &_dir) { g_baseDir = _dir; }

CpuMesh &getMesh(const std::string &_path) {
  static std::unordered_map<std::string, CpuMesh> cache;

  std::filesystem::path p(_path);
  std::filesystem::path full =
      (p.is_absolute() || _path.empty()) ? p : g_baseDir / p;
  std::string key = full.string();

  auto it = cache.find(key);
  if (it != cache.end())
    return it->second;
  auto res = cache.emplace(key, loadObj(key));
  return res.first->second;
}
} // namespace ee::core
