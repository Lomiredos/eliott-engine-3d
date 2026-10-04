#pragma once

#include "visu/render/Renderer.hpp"

#include <memory>

// ---------------------------------------------------------------------------
// Point d'entree unique pour choisir QUEL IRenderer le jeu utilise.
//
// Par defaut (si SetRenderer n'est jamais appele), GetRenderer() construit un
// GLRenderer au premier appel. Un jeu qui veut son propre backend doit
// appeler SetRenderer() AVANT le tout premier GetRenderer() (donc avant que
// quoi que ce soit touche au rendu) -- un appel plus tard est ignore (log),
// un GLRenderer par defaut etant alors deja en cours d'utilisation.
// ---------------------------------------------------------------------------

namespace ee::render
{
    void SetRenderer(std::unique_ptr<IRenderer> _renderer);
    IRenderer &GetRenderer();
}
