#pragma once

#include "visu/core/SceneInfo.hpp"

#include "visu/render/Renderer.hpp"

#include <string>
#include <vector>

namespace ee::systems {
// selected = index de l'entite dont le collider doit etre affiche en fil de
// fer (-1 = aucun contour de collider affiche).
// editorOverlays = false : rendu "comme le jeu" (pas de gizmo camera ni de
// contour de collider), ex. apercu de ce que voit une camera.
void renderScene(ee::render::IRenderer &r, const SceneInfo &scene,
                 int selected = -1, bool editorOverlays = true);

// Forme d'un collider, en valeurs simples : remplie depuis la scene (editeur)
// ou depuis le ColliderComponent typé (jeu) -> meme contour des deux cotes.
struct ColliderWireDesc {
  std::string shape = "Sphere"; // "Sphere", "Box" ou "Capsule"
  ee::render::Vec3 offset{0.0f, 0.0f, 0.0f};
  ee::render::Vec3 euler{0.0f, 0.0f, 0.0f}; // degres, apres celle du transform
  float radius = 1.0f;
  ee::render::Vec3 halfExtents{0.5f, 0.5f, 0.5f};
  float height = 2.0f;
};

// Contour fil de fer d'un collider, pour une entite placee a (pos, rotation
// en degres, echelle par axe).
void drawColliderWire(ee::render::IRenderer &r, ee::render::Vec3 pos,
                      ee::render::Vec3 eulerDeg, ee::render::Vec3 scale,
                      const ColliderWireDesc &col);

// Camera placee par un transform : position = pos, regarde vers le -Z local
// tourne par eulerDeg (meme convention que les meshes ; roulis ignore).
ee::render::Camera cameraFromTransform(ee::render::Vec3 pos,
                                       ee::render::Vec3 eulerDeg, float fovDeg,
                                       float nearZ, float farZ);

// Cameras du jeu decrites par la scene (entites CameraComponent + Transform),
// dans l'ordre de la scene = ordre des cases de l'ecran partage (voir
// splitScreen). Toutes celles avec isMain ; si aucune, la premiere seule.
struct SceneCamera {
  int entity = -1;
  ee::render::Camera cam;
};
std::vector<SceneCamera> sceneCameras(const SceneInfo &scene);

// Premiere camera de sceneCameras. false si la scene n'en a pas.
bool sceneCamera(const SceneInfo &scene, ee::render::Camera &out);

// Camera d'une entite precise (index dans la scene). false si elle n'a pas
// CameraComponent + TransformComponent.
bool entityCamera(const SceneInfo &scene, int index, ee::render::Camera &out);

// Petite camera fil de fer (boitier + objectif vers -Z local) : rend visible
// dans l'editeur une entite camera, qui n'a pas de forme a elle.
void drawCameraGizmo(ee::render::IRenderer &r, ee::render::Vec3 pos,
                     ee::render::Vec3 eulerDeg, bool selected);
}
