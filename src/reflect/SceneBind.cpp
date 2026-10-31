#include "visu/reflect/SceneBind.hpp"

#include "visu/core/SceneQuery.hpp"

#include <string>
#include <variant>

namespace ee::reflection
{
    void LoadVisitor::visit(const char *_name, int &_value)
    {
        auto it = m_comp.values.find(m_path.key(_name));
        if (it == m_comp.values.end())
            return;
        if (auto p = std::get_if<int>(&it->second))
            _value = *p;
        else if (auto p = std::get_if<float>(&it->second))
            _value = static_cast<int>(*p);
        else if (auto p = std::get_if<bool>(&it->second))
            _value = *p ? 1 : 0;
    }

    void LoadVisitor::visit(const char *_name, bool &_value)
    {
        auto it = m_comp.values.find(m_path.key(_name));
        if (it == m_comp.values.end())
            return;
        if (auto p = std::get_if<bool>(&it->second))
            _value = *p;
        else if (auto p = std::get_if<int>(&it->second))
            _value = (*p != 0);
        else if (auto p = std::get_if<float>(&it->second))
            _value = (*p != 0.0f);
    }

    void LoadVisitor::visit(const char *_name, float &_value)
    {
        _value = ee::scene::getFloat(m_comp, m_path.key(_name), _value);
    }

    void LoadVisitor::visit(const char *_name, std::string &_value)
    {
        auto it = m_comp.values.find(m_path.key(_name));
        if (it != m_comp.values.end())
            if (auto p = std::get_if<std::string>(&it->second))
                _value = *p;
    }

    void LoadVisitor::visitEnum(const char *_name, int& _value, const char* const* _labels, int _count) {
        auto it = m_comp.values.find(m_path.key(_name));
        if (it == m_comp.values.end())
            return;

        if (auto p = std::get_if<int>(&it->second)){
            if (*p >= 0 && *p < _count)
                _value = *p;
            return;
        }

        if (auto p = std::get_if<std::string>(&it->second)){
            for (int i = 0; i < _count; ++i){
                if (*p == _labels[i]){
                    _value = i;
                    return;
                }
            }
        }
    }

    // Scene : index de l'entite (-1 = aucune) -> EntityID via la table.
    void LoadVisitor::visitEntity(const char *_name, ee::EntityRef &_value, const char *)
    {
        auto it = m_comp.values.find(m_path.key(_name));
        if (it == m_comp.values.end())
            return;
        int index = -1;
        if (auto p = std::get_if<int>(&it->second))
            index = *p;
        else if (auto p = std::get_if<float>(&it->second))
            index = static_cast<int>(*p);
        else
            return;

        if (index < 0)
            _value.id = ee::EntityRef::kNone;
        else if (!m_entities)
            _value.id = static_cast<uint32_t>(index);
        else
            _value.id = index < static_cast<int>(m_entities->size()) ? (*m_entities)[index]
                                                                    : ee::EntityRef::kNone;
    }

    void SaveVisitor::visit(const char *_name, int &_value) { m_comp.values[m_path.key(_name)] = _value; }

    void SaveVisitor::visit(const char *_name, bool &_value) { m_comp.values[m_path.key(_name)] = _value; }

    void SaveVisitor::visit(const char *_name, float &_value) { m_comp.values[m_path.key(_name)] = _value; }

    void SaveVisitor::visit(const char *_name, std::string &_value) { m_comp.values[m_path.key(_name)] = _value; }


    void SaveVisitor::visitEnum(const char *_name, int &_value, const char *const *_labels, int _count)
    {
        if (_value >= 0 && _value < _count)
            m_comp.values[m_path.key(_name)] = std::string(_labels[_value]);
        else
            m_comp.values[m_path.key(_name)] = _value;
    }

    // EntityID -> index dans la scene ; -1 si aucune ou entite creee en jeu.
    void SaveVisitor::visitEntity(const char *_name, ee::EntityRef &_value, const char *)
    {
        int index = -1;
        if (_value.valid())
        {
            if (!m_entities)
                index = static_cast<int>(_value.id);
            else
                for (size_t i = 0; i < m_entities->size(); ++i)
                    if ((*m_entities)[i] == _value.id)
                    {
                        index = static_cast<int>(i);
                        break;
                    }
        }
        m_comp.values[m_path.key(_name)] = index;
    }

    void LoadVisitor::visit(const char *_name, ee::Color3 &_value)
    {
        auto it = m_comp.values.find(m_path.key(_name));
        if (it != m_comp.values.end())
            if (auto p = std::get_if<std::string>(&it->second))
                ee::fromHex(*p, _value); // texte invalide : couleur inchangee
    }

    void SaveVisitor::visit(const char *_name, ee::Color3 &_value)
    {
        m_comp.values[m_path.key(_name)] = ee::toHex(_value);
    }
}
