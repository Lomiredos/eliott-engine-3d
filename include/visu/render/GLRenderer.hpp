#pragma once

#include "visu/render/Renderer.hpp"

#include <filesystem>

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace ee::render {
class GLRenderer : public IRenderer {
public:
  ~GLRenderer() override;

  void beginScene(const Camera &cam, int width, int height) override;
  void endScene() override;
  void beginFrame(int width, int height) override;
  void beginView(const Camera &cam, const Viewport &view) override;
  MeshHandle builtin(Prim which) override;
  MeshHandle createMesh(const float *verts, int vertFloatCount,
                        const unsigned int *indices, int indexCount) override;
  using IRenderer::drawMesh; // garde le raccourci drawMesh(..., Color)
  void drawMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg,
                const Material &mat) override;
  void setLights(const Lights &lights) override;
  void setSettings(const RenderSettings &settings) override;
  TextureHandle loadTexture(const std::string &path, bool srgb) override;
  void drawWireMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg,
                    Color col) override;

  void setOffscreen(bool offscreen) { m_offscreen = offscreen; }

  // Plusieurs images offscreen avec les memes meshes (ex. vue editeur + vue
  // d'une camera) : chaque cible a son FBO ; beginScene/texture() utilisent
  // la cible courante (0 par defaut).
  void setTarget(int index);

  unsigned int texture() const { return m_targets[m_target].colorTex; }
  void viewMatrix(float out[16]) const;
  void projMatrix(float out[16]) const;

private:
  bool ensureInit();
  void ensureFbo(int w, int h);
  // (Re)compile le shader principal pour m_settings.maxPointLights lampes.
  bool buildProgram();

  MeshHandle uploadMesh(const float *verts, int vertFloatCount,
                        const unsigned int *idx, int idxCount);

  // Attache a un mesh (triangles) deja uploade un 2e jeu d'indices GL_LINES,
  // fait des seules aretes "reelles" de la grille (sans les diagonales de
  // triangulation) : c'est ce que drawWireMesh dessine pour les builtins.
  void attachWireIndices(MeshHandle mesh, const unsigned int *idx,
                         int idxCount);

  // Dessin immediat. faceSide : 0 = toutes les faces, 1 = arriere seules,
  // -1 = avant seules (transparents).
  void drawMeshNow(MeshHandle mesh, const float model[16], const Material &mat,
                   int faceSide);
  void drawWireNow(MeshHandle mesh, const float model[16], Color col);

  // Draw de la vue en cours, executes par flushView (beginView suivant,
  // endScene) : il faut tous les objets avant l'eclairage pour les ombres.
  enum class DrawKind { Opaque, Wire, Transparent };
  struct Queued {
    DrawKind kind = DrawKind::Opaque;
    MeshHandle mesh = 0;
    float model[16];
    Material mat;          // Wire : couleur dans mat.albedo
    float pos[3] = {};     // centre monde
    float radius = 0.0f;   // rayon englobant monde (tri des ombres de lampe)
    float dist2 = 0.0f;    // distance^2 a la camera (tri des transparents)
  };
  std::vector<Queued> m_queue;
  void flushView();

  // Ombres : passe de profondeur depuis le soleil / chaque face des lampes.
  void renderSunShadow(float outVP[16]);
  void renderPointShadows(const std::vector<int> &rows);
  void drawDepth(const Queued &q, const float vp[16]);
  void ensureShadowTargets();

  // Lampes de la vue : les maxPointLights les plus proches de la camera, et
  // la ligne de l'atlas d'ombres de chacune (-1 = pas d'ombre).
  std::vector<PointLight> m_activePoints;
  std::vector<int> m_activeRows;
  void selectPointLights();

  bool m_init = false;
  bool m_failed = false;
  bool m_offscreen = true;

  unsigned int m_program = 0;

  struct Mesh {
    unsigned int vao = 0, vbo = 0, ebo = 0;
    int indexCount = 0;
    unsigned int wireEbo = 0; // 0 = pas de variante fil de fer dediee
    int wireIndexCount = 0;
    float radius = 1.0f; // sommet le plus loin de l'origine (mesh local)
  };
  std::vector<Mesh> m_meshes;
  MeshHandle m_sphere = 0, m_cube = 0, m_cylinder = 0, m_hemisphere = 0;

  int m_uMVP = -1, m_uModel = -1, m_uColor = -1, m_uGlow = -1;
  // Materiau + eclairage (shader PBR).
  int m_uMetallic = -1, m_uRoughness = -1, m_uAO = -1, m_uEmissive = -1,
      m_uEmissiveIntensity = -1, m_uNormalStrength = -1, m_uUVTransform = -1,
      m_uMapMask = -1, m_uCamPos = -1, m_uAmbient = -1, m_uSunOn = -1,
      m_uSunDir = -1, m_uSunColor = -1, m_uPointCount = -1, m_uPointPos = -1,
      m_uPointColor = -1, m_uPointRange = -1, m_uTransparent = -1,
      m_uOpacity = -1, m_uFaceSide = -1, m_uSunShadowOn = -1, m_uSunVP = -1,
      m_uSunTexel = -1, m_uSunNormalBias = -1, m_uPointShadowRow = -1,
      m_uPointShadowTile = -1, m_uPointShadowTexel = -1, m_uPointNear = -1;
  void applyLights(bool sunShadow, const float sunVP[16], float sunTexelWorld);
  Lights m_lights;
  RenderSettings m_settings;
  int m_programMaxPoints = 0; // maxPointLights du shader compile
  float m_camPos[3] = {0.0f, 0.0f, 0.0f};
  float m_camFwd[3] = {0.0f, 0.0f, -1.0f};
  int m_viewGL[4] = {0, 0, 0, 0}; // viewport GL de la vue en cours
  unsigned int m_frameFbo = 0;    // FBO de l'image en cours

  // Passe de profondeur (ombres).
  unsigned int m_depthProgram = 0;
  int m_uDepthMVP = -1;
  unsigned int m_sunFbo = 0, m_sunTex = 0;
  int m_sunRes = 0;
  unsigned int m_pointFbo = 0, m_pointTex = 0;
  int m_pointTile = 0, m_pointRows = 0; // atlas : 6 faces x lignes

  struct CachedTexture {
    TextureHandle tex = 0;
    std::filesystem::file_time_type mtime{};
    std::chrono::steady_clock::time_point lastCheck{};
  };
  std::unordered_map<std::string, CachedTexture> m_textures; // cle : srgb|chemin

  struct Target {
    unsigned int fbo = 0, colorTex = 0, depthRbo = 0;
    int w = 0, h = 0;
  };
  std::vector<Target> m_targets = std::vector<Target>(1);
  int m_target = 0;

  int m_frameW = 0, m_frameH = 0; // image en cours (beginFrame)
  bool m_frameOk = false;

  float m_view[16] = {0};
  float m_proj[16] = {0};
  float m_vp[16] = {0};
};
} // namespace ee::render
