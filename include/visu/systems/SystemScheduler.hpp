#pragma once

#include "visu/core/SceneInfo.hpp"
#include "visu/core/SystemInfo.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Le PLANIFICATEUR de systemes.
//
// systems.json (via loadSystemsInDir) dit QUELS systemes tournent et dans quel
// ordre. Le REGISTRE dit ce que fait chaque nom (la fonction C++). Le jeu
// remplit le registre avec ses systemes ; le scheduler les execute chaque
// frame dans l'ordre de priorite. -> plus aucun appel de systeme en dur.
//
// Le registre est peuple par du CODE (le jeu connait ses fonctions). C'est le
// pendant runtime du "name -> system" evoque cote design : ici nom -> fonction.
// ---------------------------------------------------------------------------

namespace ee::systems
{
    // Entree de jeu NEUTRE (le backend/fenetre la remplit ; les systemes ne
    // voient que ca, jamais glfw).
    struct InputState
    {
        float moveX = 0.0f; // -1 gauche, +1 droite
        float moveZ = 0.0f; // -1 avant,  +1 arriere
    };

    // Tout ce qu'un systeme d'update peut lire pour agir sur la scene.
    struct FrameContext
    {
        SceneInfo *scene = nullptr;
        InputState input;
        float dt = 0.0f;
    };

    using SystemFn = std::function<void(const FrameContext &)>;

    // nom de systeme -> sa fonction C++.
    class SystemRegistry
    {
    public:
        void add(const std::string &_name, SystemFn _fn);
        const SystemFn *find(const std::string &_name) const;

    private:
        std::unordered_map<std::string, SystemFn> m_fns;
    };

    // Execute la liste (deja triee par priorite) via le registre.
    // Un systeme liste sans fonction enregistree est signale et ignore.
    void runSystems(const std::vector<SystemInfo> &_schedule,
                    const SystemRegistry &_registry,
                    const FrameContext &_ctx);
}
