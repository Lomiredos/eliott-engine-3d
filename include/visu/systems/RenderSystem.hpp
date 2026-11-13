#pragma once

#include "math/Quaternion.hpp"
#include "math/Vector3.hpp"
#include "visu/core/Color3.hpp"
#include "visu/core/SceneInfo.hpp"

#include "visu/render/Renderer.hpp"
#include "visu/core/MaterialStore.hpp"

#include <string>
#include <vector>

namespace ee::systems {
    void renderScene(ee::render::IRenderer& r, const SceneInfo& scene, int selected = -1, bool editorOverlays = true);

    struct ColliderWireDesc {
        std::string shape = "Sphere"; // "Sphere", "Box" ou "Capsule"
        ee::math::Vector3<float> offset;
        ee::math::Quaternion quat;
        float radius = 1.0f;
        ee::math::Vector3<float> halfExtents{0.5f, 0.5f, 0.5f};
        float height = 2.0f;
    };

    void drawColliderWire(ee::render::IRenderer& r, ee::math::Vector3<float> pos, ee::math::Quaternion quat,
                          ee::math::Vector3<float> scale, const ColliderWireDesc& col);

    ee::render::Camera cameraFromTransform(ee::math::Vector3<float> pos, ee::math::Quaternion quat, float fovDeg,
                                           float nearZ, float farZ);

    struct SceneCamera {
        int entity = -1;
        ee::render::Camera cam;
    };
    std::vector<SceneCamera> sceneCameras(const SceneInfo& scene);

    // Premiere camera de sceneCameras. false si la scene n'en a pas.
    bool sceneCamera(const SceneInfo& scene, ee::render::Camera& out);

    // Camera d'une entite precise (index dans la scene). false si elle n'a pas
    // CameraComponent + TransformComponent.
    bool entityCamera(const SceneInfo& scene, int index, ee::render::Camera& out);

    // Petite camera fil de fer (boitier + objectif vers -Z local) : rend visible
    // dans l'editeur une entite camera, qui n'a pas de forme a elle.
    void drawCameraGizmo(ee::render::IRenderer& r, ee::math::Vector3<float> pos, ee::math::Quaternion quat,
                         bool selected);

    // Materiau uni d'une couleur (formes sans MaterialComponent).
    ee::render::Material colorMaterial(ee::Color3 albedo);

    // Materiau .mat -> materiau du renderer (textures chargees via r, chemins
    // relatifs a la racine du projet).
    ee::render::Material renderMaterial(ee::render::IRenderer& r, const ee::core::MaterialAsset& asset);

    // Eclairage decrit par la scene (Directional/Point/AmbientLightComponent,
    // positions et directions monde). Sans lumiere : soleil + ambiance par defaut.
    ee::render::Lights sceneLights(const SceneInfo& scene);

    // Lumiere en fil de fer pour l'editeur : petite sphere, + fleche (soleil)
    // ou portee (ampoule selectionnee).
    void drawLightGizmo(ee::render::IRenderer& r, ee::math::Vector3<float> pos, ee::math::Quaternion quat,
                        bool directional, float range, bool selected);
} // namespace ee::systems
