#pragma once

#include "visu/core/SceneInfo.hpp"

#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Couche de REQUETE sur la scene runtime.
//
// Ici, la scene (SceneInfo) EST le World : les entites portent des composants
// dynamiques (nom + valeurs). Un systeme n'accede jamais aux structs C++ ; il
// interroge la scene par NOM de composant -- la meme semantique que la
// signature d'un systeme dans systems.json ("l'entite matche si elle a tous
// ces composants"). Render, pick et gameplay partagent ces helpers.
// ---------------------------------------------------------------------------

namespace ee::scene
{
    // Composant d'une entite par son nom, ou nullptr s'il est absent.
    const ComponentInstance *findComponent(const EntityInfo &_ent, const std::string &_name);
    ComponentInstance *findComponent(EntityInfo &_ent, const std::string &_name);

    // L'entite porte-t-elle TOUS ces composants ? = match de signature systeme.
    bool matchesSignature(const EntityInfo &_ent, const std::vector<std::string> &_signature);

    // Lecture d'un champ en float (bool/int/float -> float ; sinon _def).
    float getFloat(const ComponentInstance &_comp, const std::string &_key, float _def = 0.0f);

    // Ecriture d'un champ float (cree la cle si absente).
    void setFloat(ComponentInstance &_comp, const std::string &_key, float _value);
}
