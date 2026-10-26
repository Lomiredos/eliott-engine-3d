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
}
