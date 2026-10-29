#pragma once

// ---------------------------------------------------------------------------
// Hierarchie parent/enfant, cote World (jeu) : transforms monde des entites
// de l'ECS. La partie scene (editeur, sans ECS) est dans SceneHierarchy.hpp,
// incluse ici : inclure ce header donne les deux.
// ---------------------------------------------------------------------------

#include "visu/scene/SceneHierarchy.hpp"

#include "ecs/World.hpp"

#include "visu/components/TransformComponent.hpp"

#include <cmath>

namespace ee::scene
{
    // -----------------------------------------------------------------------
    // World (structs typees, jeu)
    // -----------------------------------------------------------------------

    // Transform monde d'une entite du World (identite sans Transform).
    // Profondeur bornee : une boucle de parents ne bloque pas le jeu.
    inline Xform worldXform(ee::ecs::World &_w, ee::ecs::EntityID _e, int _depth = 0)
    {
        if (!_w.hasComponent<TransformComponent>(_e))
            return {};
        const TransformComponent &t = *_w.getComponent<TransformComponent>(_e);
        Xform local = makeXform(t.position, t.euler, t.scale);
        if (!t.parent || t.parent.id == _e || _depth > 64 ||
            !_w.hasComponent<TransformComponent>(t.parent.id))
            return local;
        return compose(worldXform(_w, t.parent.id, _depth + 1), local);
    }

    // Deplacement exprime dans le monde -> applique au transform local de
    // _e (tient compte de la rotation/echelle de son parent).
    inline void moveWorld(ee::ecs::World &_w, ee::ecs::EntityID _e, const Vec3f &_delta)
    {
        TransformComponent &t = *_w.getComponent<TransformComponent>(_e);
        if (!t.parent || !_w.hasComponent<TransformComponent>(t.parent.id))
        {
            t.position += _delta;
            return;
        }
        Xform p = worldXform(_w, t.parent.id);
        Quat inv{p.rot.w, -p.rot.x, -p.rot.y, -p.rot.z};
        Vec3f d = inv.rotate(_delta);
        auto safeDiv = [](float _a, float _b)
        { return std::fabs(_b) > 1e-6f ? _a / _b : _a; };
        t.position += Vec3f{safeDiv(d.x, p.scale.x), safeDiv(d.y, p.scale.y), safeDiv(d.z, p.scale.z)};
    }
}
