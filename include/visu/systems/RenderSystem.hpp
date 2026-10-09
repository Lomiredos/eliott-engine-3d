#pragma once

#include "visu/core/SceneInfo.hpp"

namespace ee::render {
class IRenderer;
}

namespace ee::systems {
// selected = index de l'entite dont le collider doit etre affiche en fil de
// fer (-1 = aucun contour de collider affiche).
void renderScene(ee::render::IRenderer &r, const SceneInfo &scene,
                 int selected = -1);
}
