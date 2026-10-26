#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"
#include "visu/core/SceneInfo.hpp"

#include <cstdint>
#include <vector>


namespace ee::reflection
{
    // Index de scene -> EntityID du World (ordre de creation des entites) :
    // sert a convertir les EntityRef. nullptr = pas de World (index = id).
    using EntityTable = std::vector<uint32_t>;

    class LoadVisitor : public FieldVisitor
    {
    public:
        explicit LoadVisitor(const ComponentInstance &_comp, const EntityTable *_entities = nullptr)
            : m_comp(_comp), m_entities(_entities) {}

        void visit(const char *_name, int &_value) override;
        void visit(const char *_name, bool &_value) override;
        void visit(const char *_name, float &_value) override;
        void visit(const char *_name, std::string &_value) override;
        void visit(const char *_name, ee::math::Vector3<float> &_value) override;
        void visit(const char *_name, ee::math::Quaternion &_value) override;
        void visitEnum(const char *_name, int& _value, const char* const* _labels, int _count) override;
        void visitEntity(const char *_name, ee::EntityRef &_value, const char *_required) override;

    private:
        const ComponentInstance &m_comp;
        const EntityTable *m_entities;
    };

    // Inverse de LoadVisitor : ecrit les champs d'un composant typé dans un
    // ComponentInstance, avec les memes conventions de nommage (Vector3 ->
    // NomX/Y/Z, Quaternion -> NomW/X/Y/Z, enum -> libelle) -> aller-retour fidele.
    class SaveVisitor : public FieldVisitor
    {
    public:
        explicit SaveVisitor(ComponentInstance &_comp, const EntityTable *_entities = nullptr)
            : m_comp(_comp), m_entities(_entities) {}

        void visit(const char *_name, int &_value) override;
        void visit(const char *_name, bool &_value) override;
        void visit(const char *_name, float &_value) override;
        void visit(const char *_name, std::string &_value) override;
        void visit(const char *_name, ee::math::Vector3<float> &_value) override;
        void visit(const char *_name, ee::math::Quaternion &_value) override;
        void visitEnum(const char *_name, int& _value, const char* const* _labels, int _count) override;
        void visitEntity(const char *_name, ee::EntityRef &_value, const char *_required) override;

    private:
        ComponentInstance &m_comp;
        const EntityTable *m_entities;
    };

    template <typename T>
    T loadFromInstance(const ComponentInstance &_comp, const EntityTable *_entities = nullptr)
    {
        T out{};
        LoadVisitor v(_comp, _entities);
        Reflect<T>::visit(out, v);
        return out;
    }

    // Composant typé -> valeurs dynamiques (nom = Reflect<T>::name).
    template <typename T>
    ComponentInstance saveToInstance(T &_comp, const EntityTable *_entities = nullptr)
    {
        ComponentInstance out;
        out.name = Reflect<T>::name;
        SaveVisitor v(out, _entities);
        Reflect<T>::visit(_comp, v);
        return out;
    }

    // Applique sur un composant existant les seuls champs presents dans
    // _patch (les autres sont laisses tels quels) : modification a chaud.
    template <typename T>
    void patchFromInstance(T &_comp, const ComponentInstance &_patch,
                           const EntityTable *_entities = nullptr)
    {
        LoadVisitor v(_patch, _entities);
        Reflect<T>::visit(_comp, v);
    }
}
