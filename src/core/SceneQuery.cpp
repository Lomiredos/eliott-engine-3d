#include "visu/core/SceneQuery.hpp"

#include <variant>

namespace ee::scene
{
    const ComponentInstance *findComponent(const EntityInfo &_ent, const std::string &_name)
    {
        for (const auto &c : _ent.components)
            if (c.name == _name)
                return &c;
        return nullptr;
    }

    ComponentInstance *findComponent(EntityInfo &_ent, const std::string &_name)
    {
        for (auto &c : _ent.components)
            if (c.name == _name)
                return &c;
        return nullptr;
    }

    bool matchesSignature(const EntityInfo &_ent, const std::vector<std::string> &_signature)
    {
        for (const auto &need : _signature)
            if (!findComponent(_ent, need))
                return false;
        return true;
    }

    float getFloat(const ComponentInstance &_comp, const std::string &_key, float _def)
    {
        auto it = _comp.values.find(_key);
        if (it == _comp.values.end())
            return _def;
        if (auto p = std::get_if<float>(&it->second))
            return *p;
        if (auto p = std::get_if<int>(&it->second))
            return static_cast<float>(*p);
        if (auto p = std::get_if<bool>(&it->second))
            return *p ? 1.0f : 0.0f;
        return _def; // string -> non numerique
    }

    void setFloat(ComponentInstance &_comp, const std::string &_key, float _value)
    {
        _comp.values[_key] = _value;
    }
}
