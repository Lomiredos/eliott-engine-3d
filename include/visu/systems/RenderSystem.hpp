#pragma once

#include "visu/core/SceneInfo.hpp"

#include "visu/render/Renderer.hpp"

#include <string>

namespace ee::systems {
// selected = index de l'entite dont le collider doit etre affiche en fil de
// fer (-1 = aucun contour de collider affiche).
void renderScene(ee::render::IRenderer &r, const SceneInfo &scene,
                 int selected = -1);

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

// Camera du jeu decrite par la scene (entite CameraComponent + Transform).
// false si la scene n'en a pas. Choix : premiere isMain, sinon premiere.
bool sceneCamera(const SceneInfo &scene, ee::render::Camera &out);

// Petite camera fil de fer (boitier + objectif vers -Z local) : rend visible
// dans l'editeur une entite camera, qui n'a pas de forme a elle.
void drawCameraGizmo(ee::render::IRenderer &r, ee::render::Vec3 pos,
                     ee::render::Vec3 eulerDeg, bool selected);
}
