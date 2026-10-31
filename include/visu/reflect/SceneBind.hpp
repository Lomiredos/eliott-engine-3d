#pragma once

#include "visu/reflect/Reflect.hpp"
#include "visu/reflect/FieldVisitor.hpp"
#include "visu/core/SceneInfo.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include <cassert>


namespace ee::reflection
{
    class KeyPath {
        public:
            void push(const char *_n) {m_stack.push_back(m_stack.empty() ? _n :m_stack.back() + "." + _n);}
            void pop() {m_stack.pop_back();}
            std::string key(const char *_n) const {return m_stack.empty() ? _n : m_stack.back() + "." + _n;}
            bool empty() const {return m_stack.empty();}
        private:
            std::vector<std::string> m_stack;
    };

    using EntityTable = std::vector<uint32_t>;

    class LoadVisitor : public FieldVisitor
    {

    public:
        using FieldVisitor::visit;
        
        explicit LoadVisitor(const ComponentInstance &_comp, const EntityTable *_entities = nullptr)
            : m_comp(_comp), m_entities(_entities) {}

        void visit(const char *_name, int &_value) override;
        void visit(const char *_name, bool &_value) override;
        void visit(const char *_name, float &_value) override;
        void visit(const char *_name, std::string &_value) override;
        void visitEnum(const char *_name, int& _value, const char* const* _labels, int _count) override;
        void visitEntity(const char *_name, ee::EntityRef &_value, const char *_required) override;
        void visit(const char *_name, ee::Color3 &_value) override;

        void beginGroup(const char *_name) override { m_path.push(_name); }
        void endGroup() override {m_path.pop();}
        void beginArray(const char *_name, std::size_t &_size) override 
        {
            m_path.push(_name);
            auto it = m_comp.values.find(m_path.key("#count"));
            if (it == m_comp.values.end()) return;
            if (auto p = std::get_if<int>(&it->second); p && *p >= 0 && *p <= 100000){
                _size = static_cast<std::size_t>(*p);
            }
        }
        void endArray() override {m_path.pop();}

        bool pathEmpty() const {return m_path.empty();}
    private:


        const ComponentInstance &m_comp;
        const EntityTable *m_entities;
        KeyPath m_path;
    };

    class SaveVisitor : public FieldVisitor
    {
        
    public:
        using FieldVisitor::visit;
        explicit SaveVisitor(ComponentInstance &_comp, const EntityTable *_entities = nullptr)
            : m_comp(_comp), m_entities(_entities) {}

        void visit(const char *_name, int &_value) override;
        void visit(const char *_name, bool &_value) override;
        void visit(const char *_name, float &_value) override;
        void visit(const char *_name, ee::Color3 &_value) override;
        void visit(const char *_name, std::string &_value) override;
        void visitEnum(const char *_name, int& _value, const char* const* _labels, int _count) override;
        void visitEntity(const char *_name, ee::EntityRef &_value, const char *_required) override;

        void beginGroup(const char *_name) override { m_path.push(_name); }
        void endGroup() override { m_path.pop(); }
        void beginArray(const char *_name, std::size_t &_size) override 
        {
            m_path.push(_name);
            m_comp.values[m_path.key("#count")] = (int)_size;
        }
        void endArray() override {m_path.pop();}
        
        bool pathEmpty() const {return m_path.empty();}
    private:
        

        ComponentInstance &m_comp;
        const EntityTable *m_entities;
        KeyPath m_path;
    };

    template <typename T>
    T loadFromInstance(const ComponentInstance &_comp, const EntityTable *_entities = nullptr)
    {
        T out{};
        LoadVisitor v(_comp, _entities);
        Reflect<T>::visit(out, v);
        assert(v.pathEmpty());
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
        assert(v.pathEmpty());
        
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
        assert(v.pathEmpty());
        
    }
}
