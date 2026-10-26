#pragma once

#include <cstdio>
#include <string>

// ---------------------------------------------------------------------------
// Couleur RGB (0..1, sRGB comme un selecteur de couleur) pour les champs de
// composants : l'editeur affiche un selecteur, la scene stocke "#rrggbb".
// ---------------------------------------------------------------------------
namespace ee
{
    struct Color3
    {
        float r = 1.0f, g = 1.0f, b = 1.0f;
    };

    inline std::string toHex(const Color3 &_c)
    {
        auto byte = [](float _v)
        {
            int i = static_cast<int>(_v * 255.0f + 0.5f);
            return i < 0 ? 0 : (i > 255 ? 255 : i);
        };
        char buf[8];
        std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", byte(_c.r), byte(_c.g), byte(_c.b));
        return buf;
    }

    // "#rrggbb" (ou "rrggbb") -> couleur ; false si le texte n'en est pas une.
    inline bool fromHex(const std::string &_s, Color3 &_out)
    {
        const char *p = _s.c_str();
        if (*p == '#')
            ++p;
        unsigned int r = 0, g = 0, b = 0;
        if (std::string(p).size() != 6 || std::sscanf(p, "%02x%02x%02x", &r, &g, &b) != 3)
            return false;
        _out = {r / 255.0f, g / 255.0f, b / 255.0f};
        return true;
    }
}
