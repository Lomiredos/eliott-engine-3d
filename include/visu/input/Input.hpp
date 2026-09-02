#pragma once

// ---------------------------------------------------------------------------
// Input comme SERVICE moteur.
//
// La signature d'un systeme ECS est rigide (`update(World&, float dt)`), sans
// canal d'entree. Plutot que de tordre l'ECS, on expose l'entree du frame comme
// un service global : le jeu ecrit `ee::input::state()` chaque frame (depuis
// glfw), les systemes la lisent. Simple, et decouple les systemes de la fenetre.
// ---------------------------------------------------------------------------

namespace ee::input
{
    struct InputState
    {
        float moveX = 0.0f; // -1 gauche, +1 droite
        float moveZ = 0.0f; // -1 avant,  +1 arriere
    };

    // Etat d'entree du frame courant (unique, partagé). Le jeu ecrit, les
    // systemes lisent.
    InputState &state();
}
