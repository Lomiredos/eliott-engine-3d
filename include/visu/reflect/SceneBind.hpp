#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"
#include "visu/core/SceneInfo.hpp"


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
        void visitEnum(const char *_name, int& _value, const char* const* _labels, int _count) override;

    private:
        const ComponentInstance &m_comp;
    };

    // Inverse de LoadVisitor : ecrit les champs d'un composant typé dans un
    // ComponentInstance, avec les memes conventions de nommage (Vector3 ->
    // NomX/Y/Z, Quaternion -> NomW/X/Y/Z, enum -> libelle) -> aller-retour fidele.
    class SaveVisitor : public FieldVisitor
    {
    public:
        explicit SaveVisitor(ComponentInstance &_comp) : m_comp(_comp) {}

        void visit(const char *_name, int &_value) override;
        void visit(const char *_name, bool &_value) override;
        void visit(const char *_name, float &_value) override;
        void visit(const char *_name, std::string &_value) override;
        void visit(const char *_name, ee::math::Vector3<float> &_value) override;
        void visit(const char *_name, ee::math::Quaternion &_value) override;
        void visitEnum(const char *_name, int& _value, const char* const* _labels, int _count) override;

    private:
        ComponentInstance &m_comp;
    };

    template <typename T>
    T loadFromInstance(const ComponentInstance &_comp)
    {
        T out{};
        LoadVisitor v(_comp);
        Reflect<T>::visit(out, v);
        return out;
    }

    // Composant typé -> valeurs dynamiques (nom = Reflect<T>::name).
    template <typename T>
    ComponentInstance saveToInstance(T &_comp)
    {
        ComponentInstance out;
        out.name = Reflect<T>::name;
        SaveVisitor v(out);
        Reflect<T>::visit(_comp, v);
        return out;
    }

    // Applique sur un composant existant les seuls champs presents dans
    // _patch (les autres sont laisses tels quels) : modification a chaud.
    template <typename T>
    void patchFromInstance(T &_comp, const ComponentInstance &_patch)
    {
        LoadVisitor v(_patch);
        Reflect<T>::visit(_comp, v);
    }
}
