// Ce fichier ne fait rien a l'execution : son seul but est de forcer la
// compilation du runtime (jamais exerce avant) en l'incluant ici. Les
// fonctions visees sont inline (pas des templates) : les inclure suffit a
// les typer completement, meme sans jamais les appeler.
#include "visu/runtime/App.hpp"
#include "visu/systems/CollisionSystem.hpp"
