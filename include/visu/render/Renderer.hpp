#pragma once

#include <optional>
#include <string>
#include <vector>
#include "math/Vector3.hpp"
#include "math/Quaternion.hpp"
#include "visu/core/Color3.hpp"

namespace ee::render {

    struct Camera {
        ee::math::Vector3<float> pos;
        float yaw = 0.0f;
        float pitch = 0.0f;
        float fovDeg = 45.0f;
        float nearZ = 0.1f;
        float farZ = 200.0f;
    };

    struct Viewport {
        int x = 0, y = 0, w = 0, h = 0;
    };

    using MeshHandle = unsigned int;
    using TextureHandle = unsigned int;

    enum class MapSlot {
        Albedo,    // couleur de base (sRGB)
        Normal,    // normal map (espace tangent)
        Metallic,  // canal R
        Roughness, // canal R
        AO,        // occlusion ambiante, canal R
        Emissive,  // lumiere propre (sRGB)
        Count
    };

    struct Material {
        ee::Color3 albedo;
        float metallic = 0.0f;
        float roughness = 0.6f;
        float ao = 1.0f;
        ee::Color3 emissive{0.0f, 0.0f, 0.0f};
        float emissiveIntensity = 1.0f;
        float normalStrength = 1.0f;
        float tiling[2] = {1.0f, 1.0f};
        float offset[2] = {0.0f, 0.0f};
        TextureHandle maps[static_cast<int>(MapSlot::Count)] = {};
        bool transparent = false;
        float opacity = 1.0f;
    };

    struct PointLight {
        ee::math::Vector3<float> pos;
        ee::Color3 color;
        float intensity = 5.0f;
        float range = 10.0f;
        bool shadows = true;
    };

    struct DirectionalLight {
        ee::math::Vector3<float> dir{-0.5f, -1.0f, -0.3f};
        ee::Color3 color;
        float intensity = 2.5f;
        bool castShadows = true;
    };

    struct Lights {
        ee::Color3 ambient;
        float ambientIntensity = 0.25f;
        std::optional<DirectionalLight> sun = DirectionalLight{};
        std::vector<PointLight> points;
    };

    struct RenderSettings {

        int maxPointLights = 8;

        struct sunShadows {
            bool enable = true;
            int resolution = 2048;
            float distance = 40.f;
            bool operator==(const sunShadows& s) const = default;
        } sun;
        struct pointShadows {
            bool enable = true;
            int maxLight = 4;
            int resolution = 512;
            bool operator==(const pointShadows& s) const = default;
        } point;

        bool operator==(const RenderSettings& o) const = default;
        bool operator!=(const RenderSettings& o) const { return !(*this == o); }
    };

    enum class Prim { Sphere, Cube, Cylinder, Hemisphere };

    class IRenderer {
    public:
        virtual ~IRenderer() = default;

        // Une seule camera sur toute l'image (= beginFrame + beginView plein ecran).
        virtual void beginScene(const Camera& cam, int width, int height) = 0;
        virtual void endScene() = 0;

        // Plusieurs vues dans une image (ecran partage) : beginFrame remplit tout
        // en noir, puis chaque beginView efface sa zone et y place sa camera ;
        // les draw suivants vont dans cette zone. endScene une fois a la fin.
        virtual void beginFrame(int width, int height) = 0;
        virtual void beginView(const Camera& cam, const Viewport& view) = 0;
        // Les draw d'une vue sont executes a sa fin (beginView suivant ou
        // endScene) : ombres d'abord, puis opaques, fil de fer, transparents.

        // Eclairage des draw suivants (jusqu'au prochain setLights).
        virtual void setLights(const Lights& lights) = 0;

        // Reglages (nombre de lampes, ombres). Sans effet s'ils n'ont pas change.
        virtual void setSettings(const RenderSettings& settings) = 0;

        // Texture depuis un fichier image (png, jpg, tga, bmp...), mise en cache
        // et rechargee si le fichier change. srgb = couleur (albedo, emissive),
        // sinon donnees (normal, metallic...). 0 si illisible.
        virtual TextureHandle loadTexture(const std::string& path, bool srgb) = 0;

        virtual MeshHandle builtin(Prim which) = 0;

        virtual MeshHandle createMesh(const float* verts, int vertFloatCount, const unsigned int* indices,
                                      int indexCount) = 0;

        virtual void destroyMesh(MeshHandle mesh) = 0;
        virtual void drawMesh(MeshHandle mesh, ee::math::Vector3<float> pos, ee::math::Vector3<float> scale,
                              ee::math::Quaternion rot, const Material& mat) = 0;

        // Raccourci : materiau uni de cette couleur.
        void drawMesh(MeshHandle mesh, ee::math::Vector3<float> pos, ee::math::Vector3<float> scale,
                      ee::math::Quaternion rot, ee::Color3 col) {
            Material m;
            m.albedo = col;
            drawMesh(mesh, pos, scale, rot, m);
        }

        // Meme mesh, dessine en fil de fer (aretes) : pour les contours de debug
        // (ex. forme d'un collider), jamais pour le rendu plein des composants.
        virtual void drawWireMesh(MeshHandle mesh, ee::math::Vector3<float> pos, ee::math::Vector3<float> scale,
                                  ee::math::Quaternion rot, ee::Color3 col) = 0;
    };
} // namespace ee::render
