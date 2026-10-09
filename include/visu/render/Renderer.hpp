#pragma once

namespace ee::render {
struct Vec3 {
  float x = 0.0f, y = 0.0f, z = 0.0f;
};

struct Color {
  float r = 1.0f, g = 1.0f, b = 1.0f;
};

struct Camera {
  Vec3 pos;
  float yaw = 0.0f;
  float pitch = 0.0f;
  float fovDeg = 45.0f;
  float nearZ = 0.1f;
  float farZ = 200.0f;
};

using MeshHandle = unsigned int;

// Hemisphere = demi-sphere unitaire bombee vers +Y (dome), pour les bouts
// d'une capsule : evite d'afficher une sphere complete qui chevaucherait
// le cylindre.
enum class Prim { Sphere, Cube, Cylinder, Hemisphere };

class IRenderer {
public:
  virtual ~IRenderer() = default;

  virtual void beginScene(const Camera &cam, int width, int height) = 0;
  virtual void endScene() = 0;

  virtual MeshHandle builtin(Prim which) = 0;

  virtual MeshHandle createMesh(const float *verts, int vertFloatCount,
                                const unsigned int *indices,
                                int indexCount) = 0;

  virtual void drawMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg,
                        Color col) = 0;

  // Meme mesh, dessine en fil de fer (aretes) : pour les contours de debug
  // (ex. forme d'un collider), jamais pour le rendu plein des composants.
  virtual void drawWireMesh(MeshHandle mesh, Vec3 pos, Vec3 scale,
                            Vec3 eulerDeg, Color col) = 0;
};
} // namespace ee::render
