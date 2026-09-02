#pragma once

// ---------------------------------------------------------------------------
// Rendu depuis le World TYPE (opt-in, game-only).
//
// Miroir de ee::systems::renderScene, mais lit les structs typées du World au
// lieu des valeurs dynamiques de SceneInfo. C'est la divergence assumée :
// l'editeur previsualise SceneInfo (dynamique), le jeu rend son World (typé).
// Le backend (IRenderer/GLRenderer) reste partagé.
// ---------------------------------------------------------------------------

#include "ecs/World.hpp"

#include "visu/render/Renderer.hpp"
#include "visu/core/MeshStore.hpp"

#include "visu/components/TransformComponent.hpp"
#include "visu/components/SphereComponent.hpp"
#include "visu/components/RectComponent.hpp"
#include "visu/components/CapsuleComponent.hpp"
#include "visu/components/MeshComponent.hpp"

#include "math/Quaternion.hpp"

#include <vector>

namespace ee::scene
{
    inline void renderWorld(ee::render::IRenderer &_r, ee::ecs::World &_w,
                            const std::vector<ee::ecs::EntityID> &_ents)
    {
        for (ee::ecs::EntityID e : _ents)
        {
            if (!_w.hasComponent<TransformComponent>(e))
                continue;

            TransformComponent &t = _w.getComponent<TransformComponent>(e);
            ee::render::Vec3 pos{t.position.x, t.position.y, t.position.z};
            ee::render::Vec3 euler{t.euler.x, t.euler.y, t.euler.z};
            const float scx = t.scale.x, scy = t.scale.y, scz = t.scale.z;

            if (_w.hasComponent<SphereComponent>(e))
            {
                float radius = _w.getComponent<SphereComponent>(e).radius;
                _r.drawMesh(_r.builtin(ee::render::Prim::Sphere), pos,
                            {radius * scx, radius * scy, radius * scz}, euler,
                            ee::render::Color{0.85f, 0.20f, 0.20f});
            }

            if (_w.hasComponent<RectComponent>(e))
            {
                RectComponent &rc = _w.getComponent<RectComponent>(e);
                _r.drawMesh(_r.builtin(ee::render::Prim::Cube), pos,
                            {rc.width * scx, rc.height * scy, rc.depth * scz}, euler,
                            ee::render::Color{0.85f, 0.20f, 0.20f});
            }

            if (_w.hasComponent<CapsuleComponent>(e))
            {
                CapsuleComponent &cc = _w.getComponent<CapsuleComponent>(e);
                float rr = cc.radius * scx;
                float hh = cc.height * scy;
                ee::render::Color capCol{0.20f, 0.55f, 0.85f};

                _r.drawMesh(_r.builtin(ee::render::Prim::Cylinder), pos,
                            ee::render::Vec3{rr, hh, rr}, euler, capCol);

                ee::math::Quaternion q = ee::math::Quaternion::fromEulerDeg(euler.x, euler.y, euler.z);
                ee::math::Vector3<float> off = q.rotate({0.0f, 1.0f, 0.0f}) * (hh * 0.5f);
                ee::render::Vec3 top{pos.x + off.x, pos.y + off.y, pos.z + off.z};
                ee::render::Vec3 bot{pos.x - off.x, pos.y - off.y, pos.z - off.z};
                ee::render::Vec3 sphR{rr, rr, rr};
                _r.drawMesh(_r.builtin(ee::render::Prim::Sphere), top, sphR, euler, capCol);
                _r.drawMesh(_r.builtin(ee::render::Prim::Sphere), bot, sphR, euler, capCol);
            }

            if (_w.hasComponent<MeshComponent>(e))
            {
                ee::core::CpuMesh &cm = ee::core::getMesh(_w.getComponent<MeshComponent>(e).path);
                if (cm.valid)
                {
                    if (cm.gpu == 0)
                        cm.gpu = _r.createMesh(cm.verts.data(), (int)cm.verts.size(),
                                               cm.idx.data(), (int)cm.idx.size());
                    _r.drawMesh(cm.gpu, pos, ee::render::Vec3{scx, scy, scz}, euler,
                                ee::render::Color{0.80f, 0.80f, 0.85f});
                }
            }
        }
    }
}
