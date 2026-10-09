#include "visu/systems/RenderSystem.hpp"

#include "visu/core/MeshStore.hpp"
#include "visu/render/Renderer.hpp"

#include "math/Quaternion.hpp"

#include <cmath>
#include <string>
#include <variant>

namespace {
// Lecture d'un champ multi-type en float (bool/int/float -> float).
float valueOf(const std::map<std::string, FieldValue> &vals, const char *key,
              float def) {
  auto it = vals.find(key);
  if (it == vals.end())
    return def;
  if (auto p = std::get_if<float>(&it->second))
    return *p;
  if (auto p = std::get_if<int>(&it->second))
    return static_cast<float>(*p);
  if (auto p = std::get_if<bool>(&it->second))
    return *p ? 1.0f : 0.0f;
  return def; // string -> non numerique
}

std::string stringOf(const std::map<std::string, FieldValue> &vals,
                     const char *key) {
  auto it = vals.find(key);
  if (it != vals.end())
    if (auto p = std::get_if<std::string>(&it->second))
      return *p;
  return {};
}

// Extrait les angles d'Euler (degres, ordre X,Y,Z) d'un quaternion, selon la
// MEME convention que Quaternion::fromEulerDeg (q = qz*qy*qx) : on reste
// coherent avec rotationMat() cote GLRenderer.
ee::render::Vec3 quatToEulerDeg(const ee::math::Quaternion &q) {
  float sinPitch = 2.0f * (q.w * q.y - q.z * q.x);
  sinPitch = sinPitch < -1.0f ? -1.0f : (sinPitch > 1.0f ? 1.0f : sinPitch);
  float pitch = std::asin(sinPitch);

  float yaw = std::atan2(2.0f * (q.x * q.y + q.w * q.z),
                         1.0f - 2.0f * (q.y * q.y + q.z * q.z));
  float roll = std::atan2(2.0f * (q.y * q.z + q.w * q.x),
                          1.0f - 2.0f * (q.x * q.x + q.y * q.y));

  const float toDeg = 180.0f / 3.14159265358979323846f;
  return {roll * toDeg, pitch * toDeg, yaw * toDeg};
}
} // namespace

namespace ee::systems {
void renderScene(ee::render::IRenderer &r, const SceneInfo &scene,
                 int selected) {
  for (std::size_t idx = 0; idx < scene.entities.size(); ++idx) {
    const EntityInfo &ent = scene.entities[idx];
    const ComponentInstance *tf = nullptr;
    const ComponentInstance *sp = nullptr;
    const ComponentInstance *rp = nullptr;
    const ComponentInstance *cp = nullptr;
    const ComponentInstance *mp = nullptr;
    const ComponentInstance *cc = nullptr;
    for (const auto &ci : ent.components) {
      if (ci.name == "TransformComponent")
        tf = &ci;
      else if (ci.name == "SphereComponent")
        sp = &ci;
      else if (ci.name == "RectComponent")
        rp = &ci;
      else if (ci.name == "CapsuleComponent")
        cp = &ci;
      else if (ci.name == "MeshComponent")
        mp = &ci;
      else if (ci.name == "ColliderComponent")
        cc = &ci;
    }
    if (!tf)
      continue;

    ee::render::Vec3 pos{valueOf(tf->values, "PositionX", 0.0f),
                         valueOf(tf->values, "PositionY", 0.0f),
                         valueOf(tf->values, "PositionZ", 0.0f)};

    ee::render::Vec3 euler{valueOf(tf->values, "EuleurX", 0.0f),
                           valueOf(tf->values, "EuleurY", 0.0f),
                           valueOf(tf->values, "EuleurZ", 0.0f)};
    float scx = valueOf(tf->values, "ScaleX", 1.0f);
    float scy = valueOf(tf->values, "ScaleY", 1.0f);
    float scz = valueOf(tf->values, "ScaleZ", 1.0f);

    if (sp) {
      float radius = valueOf(sp->values, "radius", 1.0f);
      ee::render::Vec3 radii{radius * scx, radius * scy, radius * scz};
      r.drawMesh(r.builtin(ee::render::Prim::Sphere), pos, radii, euler,
                 ee::render::Color{0.85f, 0.20f, 0.20f});
    }

    if (rp) {
      ee::render::Vec3 size{valueOf(rp->values, "sizeX", 1.0f) * scx,
                            valueOf(rp->values, "sizeY", 1.0f) * scy,
                            valueOf(rp->values, "sizeZ", 1.0f) * scz};
      r.drawMesh(r.builtin(ee::render::Prim::Cube), pos, size, euler,
                 ee::render::Color{0.85f, 0.20f, 0.20f});
    }

    if (cp) {

      float rad = valueOf(cp->values, "radius", 0.5f);
      float hgt = valueOf(cp->values, "height", 1.0f);
      float rr = rad * scx;
      float hh = hgt * scy;
      ee::render::Color capCol{0.20f, 0.55f, 0.85f};

      r.drawMesh(r.builtin(ee::render::Prim::Cylinder), pos,
                 ee::render::Vec3{rr, hh, rr}, euler, capCol);

      ee::math::Quaternion q =
          ee::math::Quaternion::fromEulerDeg(euler.x, euler.y, euler.z);
      ee::math::Vector3<float> off = q.rotate({0.0f, 1.0f, 0.0f}) * (hh * 0.5f);
      ee::render::Vec3 top{pos.x + off.x, pos.y + off.y, pos.z + off.z};
      ee::render::Vec3 bot{pos.x - off.x, pos.y - off.y, pos.z - off.z};
      ee::render::Vec3 sphR{rr, rr, rr};
      r.drawMesh(r.builtin(ee::render::Prim::Sphere), top, sphR, euler, capCol);
      r.drawMesh(r.builtin(ee::render::Prim::Sphere), bot, sphR, euler, capCol);
    }

    if (mp) {
      std::string path = stringOf(mp->values, "path");
      ee::core::CpuMesh &cm = ee::core::getMesh(path);
      if (cm.valid) {
        if (cm.gpu == 0)
          cm.gpu = r.createMesh(cm.verts.data(), (int)cm.verts.size(),
                                cm.idx.data(), (int)cm.idx.size());
        r.drawMesh(cm.gpu, pos, ee::render::Vec3{scx, scy, scz}, euler,
                   ee::render::Color{0.80f, 0.80f, 0.85f});
      }
    }

    // Contour du collider : seulement pour l'entite selectionnee, jamais en
    // permanence (c'est un aide visuel d'edition, pas du gameplay).
    if (cc && static_cast<int>(idx) == selected) {
      std::string shape = stringOf(cc->values, "Shape");

      // Offset et rotation locaux du collider, dans le repere de l'entite :
      // l'offset est mis a l'echelle puis tourne par la rotation du
      // TransformComponent ; la rotation du collider se compose APRES celle
      // du transform (qWorld = qTf * qCol).
      ee::math::Vector3<float> offLocal{
          valueOf(cc->values, "OffSetX", 0.0f) * scx,
          valueOf(cc->values, "OffSetY", 0.0f) * scy,
          valueOf(cc->values, "OffSetZ", 0.0f) * scz};

      ee::math::Quaternion qTf =
          ee::math::Quaternion::fromEulerDeg(euler.x, euler.y, euler.z);
      ee::math::Quaternion qCol = ee::math::Quaternion::fromEulerDeg(
          valueOf(cc->values, "EulerX", 0.0f),
          valueOf(cc->values, "EulerY", 0.0f),
          valueOf(cc->values, "EulerZ", 0.0f));
      ee::math::Quaternion qWorld = qTf * qCol;

      ee::math::Vector3<float> offWorld = qTf.rotate(offLocal);
      ee::render::Vec3 cpos{pos.x + offWorld.x, pos.y + offWorld.y,
                            pos.z + offWorld.z};
      ee::render::Vec3 ceuler = quatToEulerDeg(qWorld);
      ee::render::Color wireCol{1.0f, 0.9f, 0.1f};

      if (shape == "Box") {
        ee::render::Vec3 size{
            valueOf(cc->values, "HalfExtentsX", 0.5f) * 2.0f * scx,
            valueOf(cc->values, "HalfExtentsY", 0.5f) * 2.0f * scy,
            valueOf(cc->values, "HalfExtentsZ", 0.5f) * 2.0f * scz};
        r.drawWireMesh(r.builtin(ee::render::Prim::Cube), cpos, size, ceuler,
                      wireCol);
      } else if (shape == "Capsule") {
        float rr = valueOf(cc->values, "radius", 1.0f) * scx;
        float hh = valueOf(cc->values, "Height", 2.0f) * scy;
        r.drawWireMesh(r.builtin(ee::render::Prim::Cylinder), cpos,
                      ee::render::Vec3{rr, hh, rr}, ceuler, wireCol);

        ee::math::Vector3<float> capOff =
            qWorld.rotate({0.0f, 1.0f, 0.0f}) * (hh * 0.5f);
        ee::render::Vec3 top{cpos.x + capOff.x, cpos.y + capOff.y,
                             cpos.z + capOff.z};
        ee::render::Vec3 bot{cpos.x - capOff.x, cpos.y - capOff.y,
                             cpos.z - capOff.z};
        ee::render::Vec3 sphR{rr, rr, rr};

        // Le dome (Hemisphere) est bombe vers +Y en local : correct tel
        // quel pour le bout du haut, il faut le retourner (180 deg sur X,
        // APRES le monde) pour qu'il bombe vers -Y pour le bout du bas.
        ee::math::Quaternion qFlip = ee::math::Quaternion::fromAxisAngle(
            {1.0f, 0.0f, 0.0f}, 3.14159265358979323846f);
        ee::render::Vec3 botEuler = quatToEulerDeg(qWorld * qFlip);

        r.drawWireMesh(r.builtin(ee::render::Prim::Hemisphere), top, sphR,
                      ceuler, wireCol);
        r.drawWireMesh(r.builtin(ee::render::Prim::Hemisphere), bot, sphR,
                      botEuler, wireCol);
      } else { // "Sphere" par defaut
        float rr = valueOf(cc->values, "radius", 1.0f);
        ee::render::Vec3 radii{rr * scx, rr * scy, rr * scz};
        r.drawWireMesh(r.builtin(ee::render::Prim::Sphere), cpos, radii,
                      ceuler, wireCol);
      }
    }
  }
}
} // namespace ee::systems
