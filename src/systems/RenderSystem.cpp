#include "visu/systems/RenderSystem.hpp"

#include "visu/render/Renderer.hpp"
#include "visu/core/MeshStore.hpp"

#include "math/Quaternion.hpp"

#include <variant>
#include <string>

namespace
{
    // Lecture d'un champ multi-type en float (bool/int/float -> float).
    float valueOf(const std::map<std::string, FieldValue> &vals, const char *key, float def)
    {
        auto it = vals.find(key);
        if (it == vals.end())
            return def;
        if (auto p = std::get_if<float>(&it->second))
            return *p;
        if (auto p = std::get_if<int>(&it->second))
            return static_cast<float>(*p);
        if (auto p = std::get_if<bool>(&it->second))
            return *p ? 1.0f : 0.0f;
        return def; // string -> non numerique
    }

    std::string stringOf(const std::map<std::string, FieldValue> &vals, const char *key)
    {
        auto it = vals.find(key);
        if (it != vals.end())
            if (auto p = std::get_if<std::string>(&it->second))
                return *p;
        return {};
    }
}

namespace ee::systems
{
    void renderScene(ee::render::IRenderer &r, const SceneInfo &scene)
    {
        for (const auto &ent : scene.entities)
        {
            const ComponentInstance *tf = nullptr;
            const ComponentInstance *sp = nullptr;
            const ComponentInstance *rp = nullptr;
            const ComponentInstance *cp = nullptr;
            const ComponentInstance *mp = nullptr;
            for (const auto &ci : ent.components)
            {
                if (ci.name == "TransformComponent")
                    tf = &ci;
                else if (ci.name == "SphereComponent")
                    sp = &ci;
                else if (ci.name == "RectComponent")
                    rp = &ci;
                else if (ci.name == "CapsuleComponent")
                    cp = &ci;
                else if (ci.name == "MeshComponent")
                    mp = &ci;
            }
            if (!tf)
                continue;

            ee::render::Vec3 pos{valueOf(tf->values, "PositionX", 0.0f),
                                 valueOf(tf->values, "PositionY", 0.0f),
                                 valueOf(tf->values, "PositionZ", 0.0f)};
            // Rotation (degres) + echelle par axe, lues sur le Transform.
            ee::render::Vec3 euler{valueOf(tf->values, "EuleurX", 0.0f),
                                   valueOf(tf->values, "EuleurY", 0.0f),
                                   valueOf(tf->values, "EuleurZ", 0.0f)};
            float scx = valueOf(tf->values, "ScaleX", 1.0f);
            float scy = valueOf(tf->values, "ScaleY", 1.0f);
            float scz = valueOf(tf->values, "ScaleZ", 1.0f);

            if (sp)
            {
                float radius = valueOf(sp->values, "radius", 1.0f);
                ee::render::Vec3 radii{radius * scx, radius * scy, radius * scz};
                r.drawMesh(r.builtin(ee::render::Prim::Sphere), pos, radii, euler,
                           ee::render::Color{0.85f, 0.20f, 0.20f});
            }

            if (rp)
            {
                ee::render::Vec3 size{valueOf(rp->values, "sizeX", 1.0f) * scx,
                                      valueOf(rp->values, "sizeY", 1.0f) * scy,
                                      valueOf(rp->values, "sizeZ", 1.0f) * scz};
                r.drawMesh(r.builtin(ee::render::Prim::Cube), pos, size, euler,
                           ee::render::Color{0.85f, 0.20f, 0.20f});
            }

            if (cp)
            {
                // Capsule = cylindre central + 2 spheres aux bouts (pas de mesh
                // dedie : on compose des primitives, les caps ne se deforment pas).
                float rad = valueOf(cp->values, "radius", 0.5f);
                float hgt = valueOf(cp->values, "height", 1.0f);
                float rr = rad * scx;  // rayon effectif
                float hh = hgt * scy;  // longueur du cylindre effective
                ee::render::Color capCol{0.20f, 0.55f, 0.85f};

                r.drawMesh(r.builtin(ee::render::Prim::Cylinder), pos,
                           ee::render::Vec3{rr, hh, rr}, euler, capCol);

                // Centres des bouchons : le long de l'axe Y tourne.
                ee::math::Quaternion q = ee::math::Quaternion::fromEulerDeg(euler.x, euler.y, euler.z);
                ee::math::Vector3<float> off = q.rotate({0.0f, 1.0f, 0.0f}) * (hh * 0.5f);
                ee::render::Vec3 top{pos.x + off.x, pos.y + off.y, pos.z + off.z};
                ee::render::Vec3 bot{pos.x - off.x, pos.y - off.y, pos.z - off.z};
                ee::render::Vec3 sphR{rr, rr, rr};
                r.drawMesh(r.builtin(ee::render::Prim::Sphere), top, sphR, euler, capCol);
                r.drawMesh(r.builtin(ee::render::Prim::Sphere), bot, sphR, euler, capCol);
            }

            if (mp)
            {
                std::string path = stringOf(mp->values, "path");
                ee::core::CpuMesh &cm = ee::core::getMesh(path);
                if (cm.valid)
                {
                    // Envoi GPU paresseux : une seule fois par chemin.
                    if (cm.gpu == 0)
                        cm.gpu = r.createMesh(cm.verts.data(), (int)cm.verts.size(),
                                              cm.idx.data(), (int)cm.idx.size());
                    r.drawMesh(cm.gpu, pos, ee::render::Vec3{scx, scy, scz}, euler,
                               ee::render::Color{0.80f, 0.80f, 0.85f});
                }
            }
        }
    }
}
