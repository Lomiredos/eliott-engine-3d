#include "visu/core/MeshStore.hpp"
#include "visu/render/MeshUtil.hpp"

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

// "p", "p/t", "p//n" ou "p/t/n" (index 1-based, ou negatifs = depuis la fin).
void parseFaceToken(const std::string &_tok, int _posCount, int _uvCount,
                    int _normCount, int &_pi, int &_ti, int &_ni) {
  _pi = _ti = _ni = -1;
  auto idx = [](const std::string &_s, int _count) {
    if (_s.empty())
      return -1;
    int v = std::stoi(_s);
    return (v > 0) ? v - 1 : _count + v;
  };
  auto slash1 = _tok.find('/');
  _pi = idx(slash1 == std::string::npos ? _tok : _tok.substr(0, slash1), _posCount);
  if (slash1 == std::string::npos)
    return;
  auto slash2 = _tok.find('/', slash1 + 1);
  _ti = idx(_tok.substr(slash1 + 1, slash2 == std::string::npos ? std::string::npos
                                                                 : slash2 - slash1 - 1),
            _uvCount);
  if (slash2 != std::string::npos)
    _ni = idx(_tok.substr(slash2 + 1), _normCount);
}

ee::core::CpuMesh loadObj(const std::string &_path) {
  ee::core::CpuMesh mesh;
  std::ifstream f(_path);
  if (!f)
    return mesh; // valid == false

  std::vector<Vec3> positions;
  std::vector<Vec3> normals;
  std::vector<std::pair<float, float>> uvs;
  std::vector<float> v8; // position, normale, uv (tangentes ajoutees a la fin)

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
      } else if (tag == "vt") {
        float u = 0.0f, w = 0.0f;
        ls >> u >> w;
        uvs.push_back({u, w});
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
          int pi[3], ti[3], ni[3];
          Vec3 pos[3];
          bool ok = true;
          for (int k = 0; k < 3; ++k) {
            parseFaceToken(*tri[k], (int)positions.size(), (int)uvs.size(),
                           (int)normals.size(), pi[k], ti[k], ni[k]);
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
            std::pair<float, float> uv =
                (ti[k] >= 0 && ti[k] < (int)uvs.size()) ? uvs[ti[k]]
                                                        : std::pair<float, float>{0.0f, 0.0f};
            v8.insert(v8.end(), {pos[k].x, pos[k].y, pos[k].z, n.x, n.y,
                                 n.z, uv.first, uv.second});
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
  mesh.verts = ee::render::withTangents(v8, mesh.idx);

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

std::filesystem::path assetPath(const std::string &_path) {
  std::filesystem::path p(_path);
  return (p.is_absolute() || _path.empty()) ? p : g_baseDir / p;
}

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
