#pragma once

#include "visu/core/SceneInfo.hpp"

namespace ee::render
{
    class IRenderer;
}

// ---------------------------------------------------------------------------
// Le "B" : la logique game-side du rendu.
//
// Parcourt les entites de la scene, lit leurs composants (Transform, Sphere)
// et les TRADUIT en appels de dessin. C'est ici que vit la connaissance de
// l'ECS -- pas dans le renderer.
//
// Point cle : ne depend QUE de IRenderer. Ce code ignore totalement si le
// backend derriere est OpenGL, raylib ou autre. On peut lui passer un
// GLRenderer (editeur) ou, demain, un RaylibRenderer (jeu autonome) : il ne
// change pas d'une ligne.
// ---------------------------------------------------------------------------

namespace ee::systems
{
    void renderScene(ee::render::IRenderer &r, const SceneInfo &scene);
}
