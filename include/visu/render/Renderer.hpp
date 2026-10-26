#pragma once

#include <string>
#include <vector>

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

// Zone de l'image, en pixels, origine en haut a gauche.
struct Viewport {
  int x = 0, y = 0, w = 0, h = 0;
};

using MeshHandle = unsigned int;
using TextureHandle = unsigned int; // 0 = aucune texture

// Les maps d'un materiau PBR (metal/rugosite), dans l'ordre des unites de
// texture du shader.
enum class MapSlot : int {
  Albedo,    // couleur de base (sRGB)
  Normal,    // normal map (espace tangent)
  Metallic,  // canal R
  Roughness, // canal R
  AO,        // occlusion ambiante, canal R
  Emissive,  // lumiere propre (sRGB)
  Count
};

// Materiau tel que le renderer le dessine (les textures deja chargees).
// Couleurs en sRGB (comme un selecteur de couleur), converties par le shader.
struct Material {
  Color albedo{1.0f, 1.0f, 1.0f};
  float metallic = 0.0f;
  float roughness = 0.6f;
  float ao = 1.0f;
  Color emissive{0.0f, 0.0f, 0.0f};
  float emissiveIntensity = 1.0f;
  float normalStrength = 1.0f;
  float tiling[2] = {1.0f, 1.0f}; // repetition des uv
  float offset[2] = {0.0f, 0.0f};
  TextureHandle maps[static_cast<int>(MapSlot::Count)] = {};
};

struct PointLight {
  Vec3 pos;
  Color color;
  float intensity = 5.0f;
  float range = 10.0f; // au-dela, plus aucune lumiere
};

// Eclairage d'une image (setLights). Par defaut : un soleil et une ambiance
// douce, pour qu'une scene sans lumiere reste lisible.
struct Lights {
  Color ambient{1.0f, 1.0f, 1.0f};
  float ambientIntensity = 0.25f;
  bool sun = true;
  Vec3 sunDir{-0.5f, -1.0f, -0.3f}; // direction dans laquelle va la lumiere
  Color sunColor{1.0f, 1.0f, 1.0f};
  float sunIntensity = 2.5f;
  std::vector<PointLight> points; // au plus kMaxPointLights utilisees
};
inline constexpr int kMaxPointLights = 8;

// Hemisphere = demi-sphere unitaire bombee vers +Y (dome), pour les bouts
// d'une capsule : evite d'afficher une sphere complete qui chevaucherait
// le cylindre.
enum class Prim { Sphere, Cube, Cylinder, Hemisphere };

class IRenderer {
public:
  virtual ~IRenderer() = default;

  // Une seule camera sur toute l'image (= beginFrame + beginView plein ecran).
  virtual void beginScene(const Camera &cam, int width, int height) = 0;
  virtual void endScene() = 0;

  // Plusieurs vues dans une image (ecran partage) : beginFrame remplit tout
  // en noir, puis chaque beginView efface sa zone et y place sa camera ;
  // les draw suivants vont dans cette zone. endScene une fois a la fin.
  virtual void beginFrame(int width, int height) = 0;
  virtual void beginView(const Camera &cam, const Viewport &view) = 0;

  // Eclairage des draw suivants (jusqu'au prochain setLights).
  virtual void setLights(const Lights &lights) = 0;

  // Texture depuis un fichier image (png, jpg, tga, bmp...), mise en cache
  // et rechargee si le fichier change. srgb = couleur (albedo, emissive),
  // sinon donnees (normal, metallic...). 0 si illisible.
  virtual TextureHandle loadTexture(const std::string &path, bool srgb) = 0;

  virtual MeshHandle builtin(Prim which) = 0;

  virtual MeshHandle createMesh(const float *verts, int vertFloatCount,
                                const unsigned int *indices,
                                int indexCount) = 0;

  virtual void drawMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg,
                        const Material &mat) = 0;

  // Raccourci : materiau uni de cette couleur.
  void drawMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg,
                Color col) {
    Material m;
    m.albedo = col;
    drawMesh(mesh, pos, scale, eulerDeg, m);
  }

  // Meme mesh, dessine en fil de fer (aretes) : pour les contours de debug
  // (ex. forme d'un collider), jamais pour le rendu plein des composants.
  virtual void drawWireMesh(MeshHandle mesh, Vec3 pos, Vec3 scale,
                            Vec3 eulerDeg, Color col) = 0;
};
} // namespace ee::render
