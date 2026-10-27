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
#include "visu/components/ColliderComponent.hpp"
#include "visu/components/CameraComponent.hpp"
#include "visu/components/LightComponents.hpp"
#include "visu/components/MaterialComponent.hpp"
#include "visu/core/MaterialStore.hpp"
#include "visu/core/EngineSettings.hpp"
#include "visu/systems/RenderSystem.hpp" // drawColliderWire
#include "visu/scene/Hierarchy.hpp"

#include "math/Quaternion.hpp"

#include <vector>

namespace ee::scene
{
    // Eclairage depuis le World : meme regles que sceneLights (editeur).
    inline ee::render::Lights worldLights(ee::ecs::World &_w, const std::vector<ee::ecs::EntityID> &_ents)
    {
        ee::render::Lights lights;
        bool anyLight = false, sunSet = false;
        for (ee::ecs::EntityID e : _ents)
        {
            if (!_w.hasComponent<TransformComponent>(e))
                continue;
            if (_w.hasComponent<DirectionalLightComponent>(e) && !sunSet)
            {
                const DirectionalLightComponent &l = *_w.getComponent<DirectionalLightComponent>(e);
                ee::math::Vector3<float> f = worldXform(_w, e).rot.forward();
                lights.sunDir = {f.x, f.y, f.z};
                lights.sunColor = {l.color.r, l.color.g, l.color.b};
                lights.sunIntensity = l.intensity;
                lights.sunShadows = l.shadows;
                sunSet = anyLight = true;
            }
            if (_w.hasComponent<PointLightComponent>(e))
            {
                const PointLightComponent &l = *_w.getComponent<PointLightComponent>(e);
                Xform wx = worldXform(_w, e);
                lights.points.push_back({{wx.pos.x, wx.pos.y, wx.pos.z},
                                         {l.color.r, l.color.g, l.color.b},
                                         l.intensity,
                                         l.range,
                                         l.shadows});
                anyLight = true;
            }
            if (_w.hasComponent<AmbientLightComponent>(e))
            {
                const AmbientLightComponent &l = *_w.getComponent<AmbientLightComponent>(e);
                lights.ambient = {l.color.r, l.color.g, l.color.b};
                lights.ambientIntensity = l.intensity;
            }
        }
        if (anyLight && !sunSet)
            lights.sun = false;
        return lights;
    }

    // _selected = index (dans _ents) de l'entite dont le collider est dessine
    // en fil de fer, -1 = aucun (aide de debug du Play dans l'editeur).
    inline void renderWorld(ee::render::IRenderer &_r, ee::ecs::World &_w,
                            const std::vector<ee::ecs::EntityID> &_ents,
                            int _selected = -1)
    {
        _r.setSettings(ee::core::engineSettings().render);
        _r.setLights(worldLights(_w, _ents));
        for (size_t idx = 0; idx < _ents.size(); ++idx)
        {
            ee::ecs::EntityID e = _ents[idx];
            if (!_w.hasComponent<TransformComponent>(e))
                continue;

            const Xform wx = worldXform(_w, e); // monde (parent resolu)
            ee::render::Vec3 pos{wx.pos.x, wx.pos.y, wx.pos.z};
            ee::render::Vec3 euler{wx.eulerDeg.x, wx.eulerDeg.y, wx.eulerDeg.z};
            const float scx = wx.scale.x, scy = wx.scale.y, scz = wx.scale.z;

            // Materiau (.mat) de l'entite, sinon couleur unie de la forme.
            const ee::core::MaterialAsset *asset =
                _w.hasComponent<MaterialComponent>(e)
                    ? ee::core::getMaterial(_w.getComponent<MaterialComponent>(e)->path)
                    : nullptr;
            auto matFor = [&](ee::render::Color _fallback)
            {
                return asset ? ee::systems::renderMaterial(_r, *asset)
                             : ee::systems::colorMaterial(_fallback);
            };

            if (_w.hasComponent<SphereComponent>(e))
            {
                float radius = _w.getComponent<SphereComponent>(e)->radius;
                _r.drawMesh(_r.builtin(ee::render::Prim::Sphere), pos,
                            {radius * scx, radius * scy, radius * scz}, euler,
                            matFor({0.85f, 0.20f, 0.20f}));
            }

            if (_w.hasComponent<RectComponent>(e))
            {
                RectComponent &rc = *_w.getComponent<RectComponent>(e);
                _r.drawMesh(_r.builtin(ee::render::Prim::Cube), pos,
                            {rc.size.x * scx, rc.size.y * scy, rc.size.z * scz}, euler,
                            matFor({0.85f, 0.20f, 0.20f}));
            }

            if (_w.hasComponent<CapsuleComponent>(e))
            {
                CapsuleComponent &cc = *_w.getComponent<CapsuleComponent>(e);
                float rr = cc.radius * scx;
                float hh = cc.height * scy;
                ee::render::Material capCol = matFor({0.20f, 0.55f, 0.85f});

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
                ee::core::CpuMesh &cm = ee::core::getMesh(_w.getComponent<MeshComponent>(e)->path);
                if (cm.valid)
                {
                    if (cm.gpu == 0)
                        cm.gpu = _r.createMesh(cm.verts.data(), (int)cm.verts.size(),
                                               cm.idx.data(), (int)cm.idx.size());
                    _r.drawMesh(cm.gpu, pos, ee::render::Vec3{scx, scy, scz}, euler,
                                matFor({0.80f, 0.80f, 0.85f}));
                }
            }

            if (static_cast<int>(idx) == _selected && _w.hasComponent<ColliderComponent>(e))
            {
                const ColliderComponent &cc = *_w.getComponent<ColliderComponent>(e);
                ee::systems::ColliderWireDesc col;
                int shape = static_cast<int>(cc.shape);
                if (shape >= 0 && shape < EnumNames<ColliderShape>::count)
                    col.shape = EnumNames<ColliderShape>::values[shape];
                col.offset = {cc.offset.x, cc.offset.y, cc.offset.z};
                col.euler = {cc.euler.x, cc.euler.y, cc.euler.z};
                col.radius = cc.radius;
                col.halfExtents = {cc.halfExtents.x, cc.halfExtents.y, cc.halfExtents.z};
                col.height = cc.height;
                ee::systems::drawColliderWire(_r, pos, euler, {scx, scy, scz}, col);
            }
        }
    }

    // Cameras du jeu depuis le World, meme regle et meme ordre que
    // sceneCameras : toutes celles avec isMain, sinon la premiere seule.
    inline std::vector<ee::render::Camera> worldCameras(ee::ecs::World &_w,
                                                        const std::vector<ee::ecs::EntityID> &_ents)
    {
        std::vector<ee::render::Camera> active, first;
        for (ee::ecs::EntityID e : _ents)
        {
            if (!_w.hasComponent<CameraComponent>(e) || !_w.hasComponent<TransformComponent>(e))
                continue;
            const CameraComponent &c = *_w.getComponent<CameraComponent>(e);
            const Xform wx = worldXform(_w, e);
            ee::render::Camera cam = ee::systems::cameraFromTransform(
                {wx.pos.x, wx.pos.y, wx.pos.z}, {wx.eulerDeg.x, wx.eulerDeg.y, wx.eulerDeg.z},
                c.fov, c.nearZ, c.farZ);
            if (c.isMain)
                active.push_back(cam);
            else if (first.empty())
                first.push_back(cam);
        }
        return active.empty() ? first : active;
    }
}
