#include "visu/reflect/SceneBind.hpp"

#include "visu/core/SceneQuery.hpp"

#include <string>
#include <variant>

namespace ee::reflection
{
    void LoadVisitor::visit(const char *_name, int &_value)
    {
        auto it = m_comp.values.find(_name);
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
        auto it = m_comp.values.find(_name);
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
        _value = ee::scene::getFloat(m_comp, _name, _value);
    }

    void LoadVisitor::visit(const char *_name, std::string &_value)
    {
        auto it = m_comp.values.find(_name);
        if (it != m_comp.values.end())
            if (auto p = std::get_if<std::string>(&it->second))
                _value = *p;
    }

    void LoadVisitor::visit(const char *_name, ee::math::Vector3<float> &_value)
    {
        std::string s(_name);
        _value.x = ee::scene::getFloat(m_comp, s + "X", _value.x);
        _value.y = ee::scene::getFloat(m_comp, s + "Y", _value.y);
        _value.z = ee::scene::getFloat(m_comp, s + "Z", _value.z);
    }

    void LoadVisitor::visit(const char *_name, ee::math::Quaternion &_value)
    {
        std::string s(_name);
        _value.w = ee::scene::getFloat(m_comp, s + "W", _value.w);
        _value.x = ee::scene::getFloat(m_comp, s + "X", _value.x);
        _value.y = ee::scene::getFloat(m_comp, s + "Y", _value.y);
        _value.z = ee::scene::getFloat(m_comp, s + "Z", _value.z);
    }

    void LoadVisitor::visitEnum(const char *_name, int& _value, const char* const* _labels, int _count) {
        auto it = m_comp.values.find(_name);
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
}
