#pragma once

#include "visu/core/SceneInfo.hpp"
#include "math/Vector3.hpp"

// ---------------------------------------------------------------------------
// Picking : quelle entite un rayon touche-t-il ?
//
// Outillage cote scene, INDEPENDANT du rendu (aucun OpenGL ici). On lui donne
// un rayon en donnees pures + la scene, il renvoie l'entite touchee la plus
// proche. La geometrie s'appuie sur ee::math.
//
// Meme esprit que RenderSystem : ce code parcourt les entites/composants, mais
// au lieu de les DESSINER il teste un rayon contre leurs formes.
//
// Ajouter une forme (Mesh, Capsule...) = ecrire sa fonction rayXxx + une ligne
// dans la table des formes, dans PickingSystem.cpp. pickScene ne bouge pas.
// ---------------------------------------------------------------------------

namespace ee::systems
{
    // Rayon en donnees pures. _dir est suppose normalise.
    struct Ray
    {
        ee::math::Vector3<float> origin;
        ee::math::Vector3<float> dir;
    };

    // Renvoie l'index de l'entite touchee la plus proche par _ray, ou -1.
    int pickScene(const SceneInfo &_scene, const Ray &_ray);
}
