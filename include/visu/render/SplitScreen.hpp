#pragma once

#include "visu/render/Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ee::render
{
    // Ecran partage entre _count cameras, dans une image _width x _height :
    // 1 camera = plein ecran, puis 2, 4, 6, 8... cases (nombre pair), en
    // grille (2 = cote a cote, 4 = 2x2, 6 = 3x2...). Les cameras remplissent
    // les cases de gauche a droite, de haut en bas ; les cases en trop
    // restent noires. Renvoie une vue par camera (origine en haut a gauche).
    inline std::vector<Viewport> splitScreen(int _count, int _width, int _height)
    {
        std::vector<Viewport> out;
        if (_count <= 1)
        {
            out.push_back({0, 0, _width, _height});
            return out;
        }

        const int cells = (_count + 1) / 2 * 2;
        const int rows = std::max(1, static_cast<int>(std::floor(std::sqrt(static_cast<float>(cells)))));
        const int cols = (cells + rows - 1) / rows;
        const int gap = 1; // liseré noir entre les vues

        for (int i = 0; i < _count; ++i)
        {
            int c = i % cols, r = i / cols;
            int x0 = _width * c / cols, x1 = _width * (c + 1) / cols;
            int y0 = _height * r / rows, y1 = _height * (r + 1) / rows;
            Viewport v;
            v.x = x0 + (c > 0 ? gap : 0);
            v.y = y0 + (r > 0 ? gap : 0);
            v.w = std::max(1, x1 - (c < cols - 1 ? gap : 0) - v.x);
            v.h = std::max(1, y1 - (r < rows - 1 ? gap : 0) - v.y);
            out.push_back(v);
        }
        return out;
    }
}
