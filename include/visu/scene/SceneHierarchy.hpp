#pragma once

// ---------------------------------------------------------------------------
// Hierarchie parent/enfant, cote scene (valeurs dynamiques, editeur) : sans
// ECS, utilisable par l'editeur seul (ee-format). Le cote World (jeu) est
// dans Hierarchy.hpp.
//
// Le parent d'une entite est le champ "Parent" de son TransformComponent
// (EntityRef). Son transform est alors LOCAL : relatif a celui du parent
// (position dans le repere du parent, rotation et echelle composees). Une
// boucle de parents (A -> B -> A) est ignoree : l'entite est traitee comme
// une racine.
// ---------------------------------------------------------------------------

#include "visu/core/SceneInfo.hpp"

#include "math/Quaternion.hpp"
#include "math/Vector3.hpp"

#include <cmath>
#include <string>
#include <variant>
#include <vector>

namespace ee::scene
{
    using Vec3f = ee::math::Vector3<float>;
    using Quat = ee::math::Quaternion;

    struct Xform
    {
        Vec3f pos{0.0f, 0.0f, 0.0f};
        Quat rot{1.0f, 0.0f, 0.0f, 0.0f};
        Vec3f scale{1.0f, 1.0f, 1.0f};
        // Rotation en degres (X, Y, Z) ; pour une racine, les valeurs du
        // transform telles quelles (pas de conversion).
        Vec3f eulerDeg{0.0f, 0.0f, 0.0f};
    };

    // Angles d'Euler (degres, X Y Z) d'un quaternion, meme convention que
    // Quaternion::fromEulerDeg (q = qz*qy*qx).
    inline Vec3f eulerDegFromQuat(const Quat &_q)
    {
        float sinPitch = 2.0f * (_q.w * _q.y - _q.z * _q.x);
        sinPitch = sinPitch < -1.0f ? -1.0f : (sinPitch > 1.0f ? 1.0f : sinPitch);
        float pitch = std::asin(sinPitch);
        float yaw = std::atan2(2.0f * (_q.x * _q.y + _q.w * _q.z),
                               1.0f - 2.0f * (_q.y * _q.y + _q.z * _q.z));
        float roll = std::atan2(2.0f * (_q.y * _q.z + _q.w * _q.x),
                                1.0f - 2.0f * (_q.x * _q.x + _q.y * _q.y));
        const float toDeg = 180.0f / 3.14159265358979323846f;
        return {roll * toDeg, pitch * toDeg, yaw * toDeg};
    }

    inline Xform makeXform(Vec3f _pos, Vec3f _eulerDeg, Vec3f _scale)
    {
        Xform x;
        x.pos = _pos;
        x.eulerDeg = _eulerDeg;
        x.rot = Quat::fromEulerDeg(_eulerDeg.x, _eulerDeg.y, _eulerDeg.z);
        x.scale = _scale;
        return x;
    }

    // Monde de l'enfant = parent * local (echelle par axe, sans cisaillement).
    inline Xform compose(const Xform &_parent, const Xform &_local)
    {
        Xform w;
        w.pos = _parent.pos + _parent.rot.rotate(_local.pos * _parent.scale);
        w.rot = _parent.rot * _local.rot;
        w.scale = _parent.scale * _local.scale;
        w.eulerDeg = eulerDegFromQuat(w.rot);
        return w;
    }

    // Inverse de compose : le local qui donne _world sous _parent.
    inline Xform relativeTo(const Xform &_parent, const Xform &_world)
    {
        auto safeDiv = [](float _a, float _b)
        { return std::fabs(_b) > 1e-6f ? _a / _b : _a; };
        Quat inv{_parent.rot.w, -_parent.rot.x, -_parent.rot.y, -_parent.rot.z};
        Vec3f p = inv.rotate(_world.pos - _parent.pos);

        Xform l;
        l.pos = {safeDiv(p.x, _parent.scale.x), safeDiv(p.y, _parent.scale.y),
                 safeDiv(p.z, _parent.scale.z)};
        l.rot = inv * _world.rot;
        l.scale = {safeDiv(_world.scale.x, _parent.scale.x), safeDiv(_world.scale.y, _parent.scale.y),
                   safeDiv(_world.scale.z, _parent.scale.z)};
        l.eulerDeg = eulerDegFromQuat(l.rot);
        return l;
    }

    // -----------------------------------------------------------------------
    // Scene (valeurs dynamiques, editeur)
    // -----------------------------------------------------------------------

    inline const ComponentInstance *findTransform(const EntityInfo &_ent)
    {
        for (const ComponentInstance &ci : _ent.components)
            if (ci.name == "TransformComponent")
                return &ci;
        return nullptr;
    }

    inline ComponentInstance *findTransform(EntityInfo &_ent)
    {
        for (ComponentInstance &ci : _ent.components)
            if (ci.name == "TransformComponent")
                return &ci;
        return nullptr;
    }

    namespace detail
    {
        inline float num(const ComponentInstance &_c, const char *_k, float _def)
        {
            auto it = _c.values.find(_k);
            if (it == _c.values.end())
                return _def;
            if (auto p = std::get_if<float>(&it->second))
                return *p;
            if (auto p = std::get_if<int>(&it->second))
                return static_cast<float>(*p);
            return _def;
        }
    }

    // Transform local d'une entite de la scene (identite sans Transform).
    inline Xform localXform(const EntityInfo &_ent)
    {
        const ComponentInstance *tf = findTransform(_ent);
        if (!tf)
            return {};
        using detail::num;
        return makeXform({num(*tf, "PositionX", 0.0f), num(*tf, "PositionY", 0.0f), num(*tf, "PositionZ", 0.0f)},
                         {num(*tf, "EuleurX", 0.0f), num(*tf, "EuleurY", 0.0f), num(*tf, "EuleurZ", 0.0f)},
                         {num(*tf, "ScaleX", 1.0f), num(*tf, "ScaleY", 1.0f), num(*tf, "ScaleZ", 1.0f)});
    }

    // Parent declare (index), -1 si aucun ou invalide (hors scene, sans Transform).
    inline int declaredParent(const SceneInfo &_scene, int _index)
    {
        const ComponentInstance *tf = findTransform(_scene.entities[_index]);
        if (!tf)
            return -1;
        auto it = tf->values.find("Parent");
        if (it == tf->values.end())
            return -1;
        const int *p = std::get_if<int>(&it->second);
        if (!p || *p < 0 || *p >= static_cast<int>(_scene.entities.size()) || *p == _index)
            return -1;
        if (!findTransform(_scene.entities[*p]))
            return -1;
        return *p;
    }

    // Parent effectif de chaque entite (-1 = racine) : les boucles sont
    // coupees (l'entite qui ferme la boucle devient racine).
    inline std::vector<int> sceneParents(const SceneInfo &_scene)
    {
        const int n = static_cast<int>(_scene.entities.size());
        std::vector<int> parents(n);
        for (int i = 0; i < n; ++i)
            parents[i] = declaredParent(_scene, i);
        for (int i = 0; i < n; ++i)
        {
            int p = parents[i];
            for (int steps = 0; p >= 0; ++steps)
            {
                if (p == i || steps > n)
                {
                    parents[i] = -1;
                    break;
                }
                p = parents[p];
            }
        }
        return parents;
    }

    // true si _candidate est _ancestor ou l'un de ses descendants.
    inline bool isInSubtree(const std::vector<int> &_parents, int _candidate, int _ancestor)
    {
        for (int p = _candidate, steps = 0; p >= 0 && steps <= static_cast<int>(_parents.size());
             p = _parents[p], ++steps)
            if (p == _ancestor)
                return true;
        return false;
    }

    // Transform monde de chaque entite de la scene.
    inline std::vector<Xform> sceneWorldXforms(const SceneInfo &_scene)
    {
        const int n = static_cast<int>(_scene.entities.size());
        std::vector<int> parents = sceneParents(_scene);
        std::vector<Xform> out(n);
        std::vector<char> done(n, 0);

        // Iteratif : on remonte jusqu'a un ancetre deja calcule.
        std::vector<int> chain;
        for (int i = 0; i < n; ++i)
        {
            chain.clear();
            for (int e = i; e >= 0 && !done[e]; e = parents[e])
                chain.push_back(e);
            for (auto it = chain.rbegin(); it != chain.rend(); ++it)
            {
                int e = *it;
                Xform local = localXform(_scene.entities[e]);
                out[e] = parents[e] >= 0 ? compose(out[parents[e]], local) : local;
                done[e] = 1;
            }
        }
        return out;
    }

    // Ecrit un transform local dans le TransformComponent (valeurs de scene).
    inline void writeLocalXform(ComponentInstance &_tf, const Xform &_local)
    {
        _tf.values["PositionX"] = _local.pos.x;
        _tf.values["PositionY"] = _local.pos.y;
        _tf.values["PositionZ"] = _local.pos.z;
        _tf.values["EuleurX"] = _local.eulerDeg.x;
        _tf.values["EuleurY"] = _local.eulerDeg.y;
        _tf.values["EuleurZ"] = _local.eulerDeg.z;
        _tf.values["ScaleX"] = _local.scale.x;
        _tf.values["ScaleY"] = _local.scale.y;
        _tf.values["ScaleZ"] = _local.scale.z;
    }

    // Change le parent de _child (-1 = racine) en GARDANT sa place dans le
    // monde (son transform local est recalcule). false si refuse : pas de
    // Transform, ou _newParent est _child lui-meme ou l'un de ses descendants.
    inline bool reparent(SceneInfo &_scene, int _child, int _newParent)
    {
        const int n = static_cast<int>(_scene.entities.size());
        if (_child < 0 || _child >= n || _newParent >= n)
            return false;
        ComponentInstance *tf = findTransform(_scene.entities[_child]);
        if (!tf)
            return false;
        std::vector<int> parents = sceneParents(_scene);
        if (_newParent >= 0 && (!findTransform(_scene.entities[_newParent]) ||
                                isInSubtree(parents, _newParent, _child)))
            return false;

        std::vector<Xform> world = sceneWorldXforms(_scene);
        Xform local = _newParent >= 0 ? relativeTo(world[_newParent], world[_child]) : world[_child];
        writeLocalXform(*tf, local);
        tf->values["Parent"] = _newParent;
        return true;
    }
}
