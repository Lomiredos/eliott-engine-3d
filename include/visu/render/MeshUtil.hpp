#pragma once

#include "math/Vector.hpp"
#include <cmath>
#include <cstddef>
#include <vector>

namespace ee::render
{
    // Format d'un sommet (createMesh) : 12 floats
    //   position (3), normale (3), uv (2), tangente (4 : xyz + signe de la
    //   bitangente, pour les normal maps).
    inline constexpr int kVertexFloats = 12;

    // Sommets "position, normale, uv" (8 floats) -> format complet (12), avec
    // des tangentes calculees depuis les uv de chaque triangle.
    inline std::vector<float> withTangents(const std::vector<float> &_v8,
                                           const std::vector<unsigned int> &_idx)
    {
        using Vec3 = ee::math::Vector<3, float>;
        const std::size_t n = _v8.size() / 8;
        std::vector<Vec3> tan(n), bit(n);

        auto posOf = [&](std::size_t _i) { return Vec3{_v8[_i * 8], _v8[_i * 8 + 1], _v8[_i * 8 + 2]}; };

        for (std::size_t t = 0; t + 2 < _idx.size(); t += 3)
        {
            const unsigned int i0 = _idx[t], i1 = _idx[t + 1], i2 = _idx[t + 2];
            if (i0 >= n || i1 >= n || i2 >= n)
                continue;
            const float *a = &_v8[i0 * 8], *b = &_v8[i1 * 8], *c = &_v8[i2 * 8];
            const Vec3 e1 = posOf(i1) - posOf(i0);
            const Vec3 e2 = posOf(i2) - posOf(i0);
            const float du1 = b[6] - a[6], dv1 = b[7] - a[7];
            const float du2 = c[6] - a[6], dv2 = c[7] - a[7];
            const float det = du1 * dv2 - du2 * dv1;
            if (std::fabs(det) < 1e-12f)
                continue; // uv degeneres : la tangente sera choisie plus bas
            const float r = 1.0f / det;
            const Vec3 T = (e1 * dv2 - e2 * dv1) * r;
            const Vec3 B = (e2 * du1 - e1 * du2) * r;
            for (unsigned int i : {i0, i1, i2})
            {
                tan[i] += T;
                bit[i] += B;
            }
        }

        std::vector<float> out;
        out.reserve(n * kVertexFloats);
        for (std::size_t i = 0; i < n; ++i)
        {
            const float *v = &_v8[i * 8];
            const Vec3 N{v[3], v[4], v[5]};
            // Gram-Schmidt : tangente perpendiculaire a la normale.
            Vec3 T = tan[i] - N * dot(N, tan[i]);
            float len = T.length();
            if (len < 1e-8f)
            {
                // Pas d'uv exploitables : n'importe quel axe perpendiculaire.
                const Vec3 ax = std::fabs(N.x) < 0.9f ? Vec3{1.0f, 0.0f, 0.0f} : Vec3{0.0f, 1.0f, 0.0f};
                T = cross(ax, N);
                len = T.length();
                if (len < 1e-8f)
                {
                    T = {1.0f, 0.0f, 0.0f};
                    len = 1.0f;
                }
            }
            T /= len;
            // Signe : la bitangente calculee est-elle du cote de N x T ?
            const float w = dot(cross(N, T), bit[i]) < 0.0f ? -1.0f : 1.0f;

            out.insert(out.end(), {v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], T.x, T.y, T.z, w});
        }
        return out;
    }
}
