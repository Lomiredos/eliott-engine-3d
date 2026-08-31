#pragma once

#include "Vector2.hpp"

namespace ee::math
{
    struct Transform
    {
        Vector2<float> position;
        Vector2<float> scale = Vector2<float>(1.f, 1.f);
        float rotation = 0;
    };
}