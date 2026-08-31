#pragma once

#include "math/Vector3.hpp"
#include "math/Quaternion.hpp"

#include <string>

// ---------------------------------------------------------------------------
// L'ensemble FERME des types que le monde data sait manipuler. Un champ de
// composant n'est serialisable/editable que si son type est ici -- ou s'il se
// ramene a ceux-ci (une struct elle-meme reflechie -> recursion).
//
// FEUILLES : int, bool, float, string.
// COMPOSITES connus : Vector3, Quaternion (aplatis en floats par le visiteur
// concret -- voir CatalogGen). Tout autre type = non serialisable.
// ---------------------------------------------------------------------------

namespace ee::reflection
{
    class FieldVisitor
    {
    public:
        virtual ~FieldVisitor() = default;

        virtual void visit(const char *_name, int &_value) = 0;
        virtual void visit(const char *_name, bool &_value) = 0;
        virtual void visit(const char *_name, float &_value) = 0;
        virtual void visit(const char *_name, std::string &_value) = 0;
        virtual void visit(const char *_name, ee::math::Vector3<float> &_value) = 0;
        virtual void visit(const char *_name, ee::math::Quaternion &_value) = 0;
    };
}
