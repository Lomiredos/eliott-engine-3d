#pragma once

#include "visu/render/Renderer.hpp"

#include <vector>

namespace ee::render {
class GLRenderer : public IRenderer {
public:
  ~GLRenderer() override;

  void beginScene(const Camera &cam, int width, int height) override;
  void endScene() override;
  MeshHandle builtin(Prim which) override;
  MeshHandle createMesh(const float *verts, int vertFloatCount,
                        const unsigned int *indices, int indexCount) override;
  void drawMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg,
                Color col) override;
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

  MeshHandle uploadMesh(const float *verts, int vertFloatCount,
                        const unsigned int *idx, int idxCount);

  // Attache a un mesh (triangles) deja uploade un 2e jeu d'indices GL_LINES,
  // fait des seules aretes "reelles" de la grille (sans les diagonales de
  // triangulation) : c'est ce que drawWireMesh dessine pour les builtins.
  void attachWireIndices(MeshHandle mesh, const unsigned int *idx,
                         int idxCount);

  bool m_init = false;
  bool m_failed = false;
  bool m_offscreen = true;

  unsigned int m_program = 0;

  struct Mesh {
    unsigned int vao = 0, vbo = 0, ebo = 0;
    int indexCount = 0;
    unsigned int wireEbo = 0; // 0 = pas de variante fil de fer dediee
    int wireIndexCount = 0;
  };
  std::vector<Mesh> m_meshes;
  MeshHandle m_sphere = 0, m_cube = 0, m_cylinder = 0, m_hemisphere = 0;

  int m_uMVP = -1, m_uModel = -1, m_uColor = -1, m_uGlow = -1;

  struct Target {
    unsigned int fbo = 0, colorTex = 0, depthRbo = 0;
    int w = 0, h = 0;
  };
  std::vector<Target> m_targets = std::vector<Target>(1);
  int m_target = 0;

  float m_view[16] = {0};
  float m_proj[16] = {0};
  float m_vp[16] = {0};
};
} // namespace ee::render
