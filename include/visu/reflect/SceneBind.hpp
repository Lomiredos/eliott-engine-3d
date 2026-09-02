#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"
#include "visu/core/SceneInfo.hpp"

// ---------------------------------------------------------------------------
// Pont scene dynamique -> struct typee.
//
// LoadVisitor lit les valeurs (map dynamique) d'un ComponentInstance et remplit
// les champs d'une struct via Reflect<T>. C'est le desérialiseur runtime :
// il transforme la donnee authoree (scene.json) en composants C++ typés qu'on
// pousse dans le World. Aplatissement inverse : un Vector3 se relit depuis
// nomX/nomY/nomZ (symetrique de CatalogGen).
// ---------------------------------------------------------------------------

namespace ee::reflection
{
    class LoadVisitor : public FieldVisitor
    {
    public:
        explicit LoadVisitor(const ComponentInstance &_comp) : m_comp(_comp) {}

        void visit(const char *_name, int &_value) override;
        void visit(const char *_name, bool &_value) override;
        void visit(const char *_name, float &_value) override;
        void visit(const char *_name, std::string &_value) override;
        void visit(const char *_name, ee::math::Vector3<float> &_value) override;
        void visit(const char *_name, ee::math::Quaternion &_value) override;

    private:
        const ComponentInstance &m_comp;
    };

    // Construit un T typé depuis un ComponentInstance (defauts de T{} si champ absent).
    template <typename T>
    T loadFromInstance(const ComponentInstance &_comp)
    {
        T out{};
        LoadVisitor v(_comp);
        Reflect<T>::visit(out, v);
        return out;
    }
}
