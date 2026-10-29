#include "visu/systems/RenderSystem.hpp"

#include "visu/core/MeshStore.hpp"
#include "visu/render/Renderer.hpp"
#include "visu/scene/SceneHierarchy.hpp"
#include "visu/core/MaterialStore.hpp"
#include "visu/core/EngineSettings.hpp"

#include "math/Quaternion.hpp"

#include <algorithm>
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
                 int selected, bool editorOverlays) {
  // Transforms monde (hierarchie parent/enfant resolue).
  const std::vector<ee::scene::Xform> world = ee::scene::sceneWorldXforms(scene);
  r.setSettings(ee::core::engineSettings().render);
  r.setLights(sceneLights(scene));
  for (std::size_t idx = 0; idx < scene.entities.size(); ++idx) {
    const EntityInfo &ent = scene.entities[idx];
    const ComponentInstance *tf = nullptr;
    const ComponentInstance *sp = nullptr;
    const ComponentInstance *rp = nullptr;
    const ComponentInstance *cp = nullptr;
    const ComponentInstance *mp = nullptr;
    const ComponentInstance *cc = nullptr;
    const ComponentInstance *cam = nullptr;
    const ComponentInstance *matc = nullptr;
    const ComponentInstance *dirLight = nullptr;
    const ComponentInstance *pointLight = nullptr;
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
      else if (ci.name == "CameraComponent")
        cam = &ci;
      else if (ci.name == "MaterialComponent")
        matc = &ci;
      else if (ci.name == "DirectionalLightComponent")
        dirLight = &ci;
      else if (ci.name == "PointLightComponent")
        pointLight = &ci;
    }
    if (!tf)
      continue;

    const ee::scene::Xform &wx = world[idx];
    ee::render::Vec3 pos{wx.pos.x, wx.pos.y, wx.pos.z};
    ee::render::Vec3 euler{wx.eulerDeg.x, wx.eulerDeg.y, wx.eulerDeg.z};
    float scx = wx.scale.x;
    float scy = wx.scale.y;
    float scz = wx.scale.z;

    // Materiau de l'entite (.mat), sinon couleur unie propre a chaque forme.
    const ee::core::MaterialAsset *asset =
        matc ? ee::core::getMaterial(stringOf(matc->values, "path")) : nullptr;
    auto matFor = [&](ee::render::Color _fallback) {
      return asset ? renderMaterial(r, *asset) : colorMaterial(_fallback);
    };

    if (sp) {
      float radius = valueOf(sp->values, "radius", 1.0f);
      ee::render::Vec3 radii{radius * scx, radius * scy, radius * scz};
      r.drawMesh(r.builtin(ee::render::Prim::Sphere), pos, radii, euler,
                 matFor({0.85f, 0.20f, 0.20f}));
    }

    if (rp) {
      ee::render::Vec3 size{valueOf(rp->values, "sizeX", 1.0f) * scx,
                            valueOf(rp->values, "sizeY", 1.0f) * scy,
                            valueOf(rp->values, "sizeZ", 1.0f) * scz};
      r.drawMesh(r.builtin(ee::render::Prim::Cube), pos, size, euler,
                 matFor({0.85f, 0.20f, 0.20f}));
    }

    if (cp) {

      float rad = valueOf(cp->values, "radius", 0.5f);
      float hgt = valueOf(cp->values, "height", 1.0f);
      float rr = rad * scx;
      float hh = hgt * scy;
      ee::render::Material capCol = matFor({0.20f, 0.55f, 0.85f});

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
                   matFor({0.80f, 0.80f, 0.85f}));
      }
    }

    if (editorOverlays && pointLight)
      drawLightGizmo(r, pos, euler, false,
                     valueOf(pointLight->values, "range", 10.0f),
                     static_cast<int>(idx) == selected);
    if (editorOverlays && dirLight)
      drawLightGizmo(r, pos, euler, true, 0.0f,
                     static_cast<int>(idx) == selected);

    if (cam && editorOverlays)
      drawCameraGizmo(r, pos, euler, static_cast<int>(idx) == selected);

    // Contour du collider : seulement pour l'entite selectionnee, jamais en
    // permanence (c'est un aide visuel d'edition, pas du gameplay).
    if (cc && editorOverlays && static_cast<int>(idx) == selected) {
      ColliderWireDesc col;
      col.shape = stringOf(cc->values, "Shape");
      col.offset = {valueOf(cc->values, "OffSetX", 0.0f),
                    valueOf(cc->values, "OffSetY", 0.0f),
                    valueOf(cc->values, "OffSetZ", 0.0f)};
      col.euler = {valueOf(cc->values, "EulerX", 0.0f),
                   valueOf(cc->values, "EulerY", 0.0f),
                   valueOf(cc->values, "EulerZ", 0.0f)};
      col.radius = valueOf(cc->values, "radius", 1.0f);
      col.halfExtents = {valueOf(cc->values, "HalfExtentsX", 0.5f),
                         valueOf(cc->values, "HalfExtentsY", 0.5f),
                         valueOf(cc->values, "HalfExtentsZ", 0.5f)};
      col.height = valueOf(cc->values, "Height", 2.0f);
      drawColliderWire(r, pos, euler, {scx, scy, scz}, col);
    }
  }
}

void drawColliderWire(ee::render::IRenderer &r, ee::render::Vec3 pos,
                      ee::render::Vec3 euler, ee::render::Vec3 scale,
                      const ColliderWireDesc &col) {
  const float scx = scale.x, scy = scale.y, scz = scale.z;

  // Offset et rotation locaux du collider, dans le repere de l'entite :
  // l'offset est mis a l'echelle puis tourne par la rotation du
  // TransformComponent ; la rotation du collider se compose APRES celle
  // du transform (qWorld = qTf * qCol).
  ee::math::Vector3<float> offLocal{col.offset.x * scx, col.offset.y * scy,
                                    col.offset.z * scz};

  ee::math::Quaternion qTf =
      ee::math::Quaternion::fromEulerDeg(euler.x, euler.y, euler.z);
  ee::math::Quaternion qCol =
      ee::math::Quaternion::fromEulerDeg(col.euler.x, col.euler.y, col.euler.z);
  ee::math::Quaternion qWorld = qTf * qCol;

  ee::math::Vector3<float> offWorld = qTf.rotate(offLocal);
  ee::render::Vec3 cpos{pos.x + offWorld.x, pos.y + offWorld.y,
                        pos.z + offWorld.z};
  ee::render::Vec3 ceuler = quatToEulerDeg(qWorld);
  ee::render::Color wireCol{1.0f, 0.9f, 0.1f};

  if (col.shape == "Box") {
    ee::render::Vec3 size{col.halfExtents.x * 2.0f * scx,
                          col.halfExtents.y * 2.0f * scy,
                          col.halfExtents.z * 2.0f * scz};
    r.drawWireMesh(r.builtin(ee::render::Prim::Cube), cpos, size, ceuler,
                   wireCol);
  } else if (col.shape == "Capsule") {
    float rr = col.radius * scx;
    float hh = col.height * scy;
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

    r.drawWireMesh(r.builtin(ee::render::Prim::Hemisphere), top, sphR, ceuler,
                   wireCol);
    r.drawWireMesh(r.builtin(ee::render::Prim::Hemisphere), bot, sphR,
                   botEuler, wireCol);
  } else { // "Sphere" par defaut
    ee::render::Vec3 radii{col.radius * scx, col.radius * scy,
                           col.radius * scz};
    r.drawWireMesh(r.builtin(ee::render::Prim::Sphere), cpos, radii, ceuler,
                   wireCol);
  }
}
} // namespace ee::systems

namespace ee::systems {
ee::render::Camera cameraFromTransform(ee::render::Vec3 pos,
                                       ee::render::Vec3 eulerDeg, float fovDeg,
                                       float nearZ, float farZ) {
  ee::math::Quaternion q =
      ee::math::Quaternion::fromEulerDeg(eulerDeg.x, eulerDeg.y, eulerDeg.z);
  ee::math::Vector3<float> f = q.forward(); // -Z local
  float fy = f.y < -1.0f ? -1.0f : (f.y > 1.0f ? 1.0f : f.y);

  ee::render::Camera cam;
  cam.pos = pos;
  cam.yaw = std::atan2(f.z, f.x); // convention Camera : avant = (cos, ., sin)
  cam.pitch = std::asin(fy);
  // Valeurs bornees : nearZ <= 0 (ou fov hors de ]0,180[) casse la
  // perspective et la camera ne voit plus rien.
  cam.fovDeg = std::clamp(fovDeg, 1.0f, 179.0f);
  cam.nearZ = std::max(nearZ, 0.01f);
  cam.farZ = std::max(farZ, cam.nearZ + 0.01f);
  return cam;
}

bool entityCamera(const SceneInfo &scene, int index, ee::render::Camera &out) {
  if (index < 0 || index >= static_cast<int>(scene.entities.size()))
    return false;
  const ComponentInstance *tf = nullptr;
  const ComponentInstance *cam = nullptr;
  for (const auto &ci : scene.entities[index].components) {
    if (ci.name == "TransformComponent")
      tf = &ci;
    else if (ci.name == "CameraComponent")
      cam = &ci;
  }
  if (!tf || !cam)
    return false;
  // Position/rotation MONDE (une camera peut etre l'enfant d'une entite).
  ee::scene::Xform wx = ee::scene::sceneWorldXforms(scene)[index];
  out = cameraFromTransform({wx.pos.x, wx.pos.y, wx.pos.z},
                            {wx.eulerDeg.x, wx.eulerDeg.y, wx.eulerDeg.z},
                            valueOf(cam->values, "fov", 45.0f),
                            valueOf(cam->values, "nearZ", 0.1f),
                            valueOf(cam->values, "farZ", 200.0f));
  return true;
}

std::vector<SceneCamera> sceneCameras(const SceneInfo &scene) {
  std::vector<SceneCamera> active, first;
  for (int i = 0; i < static_cast<int>(scene.entities.size()); ++i) {
    SceneCamera sc{i, {}};
    if (!entityCamera(scene, i, sc.cam))
      continue;
    bool isMain = true;
    for (const auto &ci : scene.entities[i].components)
      if (ci.name == "CameraComponent")
        isMain = valueOf(ci.values, "isMain", 1.0f) != 0.0f;
    if (isMain)
      active.push_back(sc);
    else if (first.empty())
      first.push_back(sc);
  }
  return active.empty() ? first : active;
}

bool sceneCamera(const SceneInfo &scene, ee::render::Camera &out) {
  std::vector<SceneCamera> cams = sceneCameras(scene);
  if (cams.empty())
    return false;
  out = cams.front().cam;
  return true;
}

void drawCameraGizmo(ee::render::IRenderer &r, ee::render::Vec3 pos,
                     ee::render::Vec3 eulerDeg, bool selected) {
  ee::render::Color col = selected ? ee::render::Color{1.0f, 0.9f, 0.1f}
                                   : ee::render::Color{0.55f, 0.75f, 1.0f};
  // Boitier, centre sur l'entite.
  r.drawWireMesh(r.builtin(ee::render::Prim::Cube), pos,
                 ee::render::Vec3{0.4f, 0.3f, 0.5f}, eulerDeg, col);

  // Objectif : cylindre (axe Y) couche le long du -Z local, devant le boitier.
  ee::math::Quaternion q =
      ee::math::Quaternion::fromEulerDeg(eulerDeg.x, eulerDeg.y, eulerDeg.z);
  ee::math::Quaternion qLens = q * ee::math::Quaternion::fromEulerDeg(90.0f, 0.0f, 0.0f);
  ee::math::Vector3<float> off = q.forward() * 0.35f;
  r.drawWireMesh(r.builtin(ee::render::Prim::Cylinder),
                 ee::render::Vec3{pos.x + off.x, pos.y + off.y, pos.z + off.z},
                 ee::render::Vec3{0.12f, 0.2f, 0.12f}, quatToEulerDeg(qLens), col);
}
} // namespace ee::systems

namespace ee::systems {
ee::render::Material colorMaterial(ee::render::Color albedo) {
  ee::render::Material m;
  m.albedo = albedo;
  return m;
}

ee::render::Material renderMaterial(ee::render::IRenderer &r,
                                    const ee::core::MaterialAsset &a) {
  ee::render::Material m;
  m.albedo = {a.albedo.r, a.albedo.g, a.albedo.b};
  m.metallic = a.metallic;
  m.roughness = a.roughness;
  m.ao = a.ao;
  m.emissive = {a.emissive.r, a.emissive.g, a.emissive.b};
  m.emissiveIntensity = a.emissiveIntensity;
  m.normalStrength = a.normalStrength;
  m.tiling[0] = a.tiling[0];
  m.tiling[1] = a.tiling[1];
  m.offset[0] = a.offset[0];
  m.offset[1] = a.offset[1];
  m.transparent = a.transparent;
  m.opacity = a.opacity;
  for (int i = 0; i < ee::core::kMaterialMapCount; ++i) {
    if (a.maps[i].empty())
      continue;
    // Couleur (albedo, emissive) en sRGB ; le reste = donnees lineaires.
    bool srgb = i == static_cast<int>(ee::render::MapSlot::Albedo) ||
                i == static_cast<int>(ee::render::MapSlot::Emissive);
    m.maps[i] = r.loadTexture(ee::core::assetPath(a.maps[i]).string(), srgb);
  }
  return m;
}

ee::render::Lights sceneLights(const SceneInfo &scene) {
  ee::render::Lights lights; // defaut : soleil + ambiance
  const std::vector<ee::scene::Xform> world = ee::scene::sceneWorldXforms(scene);
  bool anyLight = false, sunSet = false;
  for (size_t i = 0; i < scene.entities.size(); ++i) {
    if (!ee::scene::findTransform(scene.entities[i]))
      continue;
    for (const auto &ci : scene.entities[i].components) {
      ee::Color3 c;
      ee::fromHex(stringOf(ci.values, "color"), c);
      ee::render::Color col{c.r, c.g, c.b};
      if (ci.name == "DirectionalLightComponent" && !sunSet) {
        ee::math::Vector3<float> f = world[i].rot.forward();
        lights.sunDir = {f.x, f.y, f.z};
        lights.sunColor = col;
        lights.sunIntensity = valueOf(ci.values, "intensity", 2.5f);
        lights.sunShadows = valueOf(ci.values, "shadows", 1.0f) != 0.0f;
        sunSet = anyLight = true;
      } else if (ci.name == "PointLightComponent") {
        lights.points.push_back({{world[i].pos.x, world[i].pos.y, world[i].pos.z},
                                 col,
                                 valueOf(ci.values, "intensity", 5.0f),
                                 valueOf(ci.values, "range", 10.0f),
                                 valueOf(ci.values, "shadows", 1.0f) != 0.0f});
        anyLight = true;
      } else if (ci.name == "AmbientLightComponent") {
        lights.ambient = col;
        lights.ambientIntensity = valueOf(ci.values, "intensity", 0.25f);
      }
    }
  }
  // Des lumieres posees a la main : plus de soleil par defaut, sauf le leur.
  if (anyLight && !sunSet)
    lights.sun = false;
  return lights;
}

void drawLightGizmo(ee::render::IRenderer &r, ee::render::Vec3 pos,
                    ee::render::Vec3 eulerDeg, bool directional, float range,
                    bool selected) {
  ee::render::Color col = selected ? ee::render::Color{1.0f, 0.9f, 0.1f}
                                   : ee::render::Color{1.0f, 0.85f, 0.4f};
  r.drawWireMesh(r.builtin(ee::render::Prim::Sphere), pos,
                 ee::render::Vec3{0.2f, 0.2f, 0.2f}, eulerDeg, col);
  if (directional) {
    // Fleche : un long cylindre fin dans la direction de la lumiere (-Z local).
    ee::math::Quaternion q =
        ee::math::Quaternion::fromEulerDeg(eulerDeg.x, eulerDeg.y, eulerDeg.z);
    ee::math::Quaternion qRay = q * ee::math::Quaternion::fromEulerDeg(90.0f, 0.0f, 0.0f);
    ee::math::Vector3<float> off = q.forward() * 0.7f;
    r.drawWireMesh(r.builtin(ee::render::Prim::Cylinder),
                   ee::render::Vec3{pos.x + off.x, pos.y + off.y, pos.z + off.z},
                   ee::render::Vec3{0.03f, 1.0f, 0.03f}, quatToEulerDeg(qRay), col);
  } else if (selected && range > 0.0f) {
    // Portee de l'ampoule.
    r.drawWireMesh(r.builtin(ee::render::Prim::Sphere), pos,
                   ee::render::Vec3{range, range, range}, eulerDeg,
                   ee::render::Color{1.0f, 0.85f, 0.4f});
  }
}
} // namespace ee::systems
