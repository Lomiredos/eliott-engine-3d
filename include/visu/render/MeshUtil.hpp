#pragma once

#include <cmath>
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
        const size_t n = _v8.size() / 8;
        std::vector<float> tan(n * 3, 0.0f), bit(n * 3, 0.0f);

        for (size_t t = 0; t + 2 < _idx.size(); t += 3)
        {
            const unsigned int i0 = _idx[t], i1 = _idx[t + 1], i2 = _idx[t + 2];
            if (i0 >= n || i1 >= n || i2 >= n)
                continue;
            const float *a = &_v8[i0 * 8], *b = &_v8[i1 * 8], *c = &_v8[i2 * 8];
            float e1[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
            float e2[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
            float du1 = b[6] - a[6], dv1 = b[7] - a[7];
            float du2 = c[6] - a[6], dv2 = c[7] - a[7];
            float det = du1 * dv2 - du2 * dv1;
            if (std::fabs(det) < 1e-12f)
                continue; // uv degeneres : la tangente sera choisie plus bas
            float r = 1.0f / det;
            float T[3], B[3];
            for (int k = 0; k < 3; ++k)
            {
                T[k] = (e1[k] * dv2 - e2[k] * dv1) * r;
                B[k] = (e2[k] * du1 - e1[k] * du2) * r;
            }
            for (unsigned int i : {i0, i1, i2})
                for (int k = 0; k < 3; ++k)
                {
                    tan[i * 3 + k] += T[k];
                    bit[i * 3 + k] += B[k];
                }
        }

        std::vector<float> out;
        out.reserve(n * kVertexFloats);
        for (size_t i = 0; i < n; ++i)
        {
            const float *v = &_v8[i * 8];
            float N[3] = {v[3], v[4], v[5]};
            float T[3] = {tan[i * 3], tan[i * 3 + 1], tan[i * 3 + 2]};
            // Gram-Schmidt : tangente perpendiculaire a la normale.
            float nd = N[0] * T[0] + N[1] * T[1] + N[2] * T[2];
            for (int k = 0; k < 3; ++k)
                T[k] -= N[k] * nd;
            float len = std::sqrt(T[0] * T[0] + T[1] * T[1] + T[2] * T[2]);
            if (len < 1e-8f)
            {
                // Pas d'uv exploitables : n'importe quel axe perpendiculaire.
                float ax[3] = {std::fabs(N[0]) < 0.9f ? 1.0f : 0.0f, std::fabs(N[0]) < 0.9f ? 0.0f : 1.0f, 0.0f};
                T[0] = ax[1] * N[2] - ax[2] * N[1];
                T[1] = ax[2] * N[0] - ax[0] * N[2];
                T[2] = ax[0] * N[1] - ax[1] * N[0];
                len = std::sqrt(T[0] * T[0] + T[1] * T[1] + T[2] * T[2]);
                if (len < 1e-8f)
                {
                    T[0] = 1.0f;
                    T[1] = T[2] = 0.0f;
                    len = 1.0f;
                }
            }
            for (int k = 0; k < 3; ++k)
                T[k] /= len;
            // Signe : la bitangente calculee est-elle du cote de N x T ?
            float C[3] = {N[1] * T[2] - N[2] * T[1], N[2] * T[0] - N[0] * T[2], N[0] * T[1] - N[1] * T[0]};
            float w = (C[0] * bit[i * 3] + C[1] * bit[i * 3 + 1] + C[2] * bit[i * 3 + 2]) < 0.0f ? -1.0f : 1.0f;

            out.insert(out.end(), {v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], T[0], T[1], T[2], w});
        }
        return out;
    }
}
