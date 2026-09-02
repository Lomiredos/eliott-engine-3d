#pragma once

#include "visu/core/SceneInfo.hpp"

#include <string>
#include <vector>


namespace ee::scene
{
    const ComponentInstance *findComponent(const EntityInfo &_ent, const std::string &_name);
    ComponentInstance *findComponent(EntityInfo &_ent, const std::string &_name);

    bool matchesSignature(const EntityInfo &_ent, const std::vector<std::string> &_signature);

    float getFloat(const ComponentInstance &_comp, const std::string &_key, float _def = 0.0f);

    void setFloat(ComponentInstance &_comp, const std::string &_key, float _value);
}
