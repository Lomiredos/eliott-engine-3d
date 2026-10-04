#include "visu/systems/PickingSystem.hpp"

#include "visu/core/MeshStore.hpp"
#include "math/Quaternion.hpp"

#include <cmath>
#include <algorithm>
#include <map>
#include <string>
#include <variant>

namespace
{
    using Vec3 = ee::math::Vector3<float>;
    using Quat = ee::math::Quaternion;

    // Lecture d'un champ multi-type en float (bool/int/float -> float).
    float valueOf(const std::map<std::string, FieldValue> &_vals, const char *_key, float _def)
    {
        auto it = _vals.find(_key);
        if (it == _vals.end())
            return _def;
        if (auto p = std::get_if<float>(&it->second))
            return *p;
        if (auto p = std::get_if<int>(&it->second))
            return static_cast<float>(*p);
        if (auto p = std::get_if<bool>(&it->second))
            return *p ? 1.0f : 0.0f;
        return _def; // string -> non numerique
    }

    std::string stringOf(const std::map<std::string, FieldValue> &_vals, const char *_key)
    {
        auto it = _vals.find(_key);
        if (it != _vals.end())
            if (auto p = std::get_if<std::string>(&it->second))
                return *p;
        return {};
    }

    // Test analytique rayon / sphere uniforme (rayon _R). _D suppose unitaire.
    bool sphereT(const Vec3 &_O, const Vec3 &_D, const Vec3 &_C, float _R, float &_t)
    {
        Vec3 oc = _O - _C;
        float b = _D.Dot(oc);
        float c = oc.Dot(oc) - _R * _R;
        float disc = b * b - c;
        if (disc <= 0.0f)
            return false;
        float tt = -b - std::sqrt(disc);
        if (tt <= 0.0f)
            return false;
        _t = tt;
        return true;
    }

    // Contrat d'un test rayon/forme.
    //   _ray    : le rayon (origine + direction normalisee)
    //   _center : position monde de la forme (son TransformComponent)
    //   _rot    : orientation monde de la forme (depuis rotX/rotY/rotZ)
    //   _shape  : le composant forme (on y lit radius, width, ...)
    //   _outT   : rempli avec la distance d'entree si touche
    // Renvoie true si le rayon touche la forme devant la camera (_outT > 0).
    using RaycastFn = bool (*)(const ee::systems::Ray &_ray,
                               const Vec3 &_center,
                               const Quat &_rot,
                               const Vec3 &_scale,
                               const ComponentInstance &_shape,
                               float &_outT);

    // --- rayon / sphere ou ellipsoide ---
    // Scale non-uniforme -> ellipsoide. On ramene le rayon en local (comme l'OBB)
    // puis on divise par les demi-axes : l'ellipsoide redevient une sphere unite,
    // d'ou une equation du 2nd degre generale (a != 1 car la direction n'est plus
    // unitaire, mais le t obtenu reste la distance monde).
    bool raySphere(const ee::systems::Ray &_ray, const Vec3 &_center,
                   const Quat &_rot, const Vec3 &_scale, const ComponentInstance &_shape, float &_outT)
    {
        float R = valueOf(_shape.values, "radius", 1.0f);
        Vec3 radii{R * _scale.x, R * _scale.y, R * _scale.z};

        Quat inv{_rot.w, -_rot.x, -_rot.y, -_rot.z};
        Vec3 ol = inv.rotate(_ray.origin - _center);
        Vec3 dl = inv.rotate(_ray.dir);
        Vec3 o{ol.x / radii.x, ol.y / radii.y, ol.z / radii.z};
        Vec3 d{dl.x / radii.x, dl.y / radii.y, dl.z / radii.z};

        float a = d.Dot(d);
        float b = 2.0f * o.Dot(d);
        float c = o.Dot(o) - 1.0f;
        float disc = b * b - 4.0f * a * c;
        if (disc <= 0.0f || a <= 0.0f)
            return false;

        _outT = (-b - std::sqrt(disc)) / (2.0f * a); // racine proche = face avant
        return _outT > 0.0f;
    }

    // --- rayon / boite orientee (OBB) ---
    // Astuce : on ramene le rayon dans le repere LOCAL de la boite (rotation
    // inverse), ce qui la rend alignee sur les axes -> simple test de slabs.
    // La rotation preserve les distances, donc la distance d'entree locale est
    // aussi la distance monde.
    bool rayBox(const ee::systems::Ray &_ray, const Vec3 &_center,
                const Quat &_rot, const Vec3 &_scale, const ComponentInstance &_shape, float &_outT)
    {
        float w = valueOf(_shape.values, "sizeX", 1.0f) * _scale.x;
        float h = valueOf(_shape.values, "sizeY", 1.0f) * _scale.y;
        float d = valueOf(_shape.values, "sizeZ", 1.0f) * _scale.z;

        // Rotation inverse = conjugue (quaternion unitaire).
        Quat inv{_rot.w, -_rot.x, -_rot.y, -_rot.z};
        Vec3 ol = inv.rotate(_ray.origin - _center); // origine en local
        Vec3 dl = inv.rotate(_ray.dir);              // direction en local

        float hx = w * 0.5f, hy = h * 0.5f, hz = d * 0.5f;

        float tx1 = (-hx - ol.x) / dl.x;
        float tx2 = (hx - ol.x) / dl.x;
        float ex = std::min(tx1, tx2), sx = std::max(tx1, tx2);

        float ty1 = (-hy - ol.y) / dl.y;
        float ty2 = (hy - ol.y) / dl.y;
        float ey = std::min(ty1, ty2), sy = std::max(ty1, ty2);

        float tz1 = (-hz - ol.z) / dl.z;
        float tz2 = (hz - ol.z) / dl.z;
        float ez = std::min(tz1, tz2), sz = std::max(tz1, tz2);

        float tEntree = std::max(ex, std::max(ey, ez));
        float tSortie = std::min(sx, std::min(sy, sz));

        if (tEntree > 0.0f && tEntree < tSortie)
        {
            _outT = tEntree;
            return true;
        }
        return false;
    }

    // --- rayon / mesh (approxime par sa boite englobante AABB) ---
    // On ramene le rayon en local (rotation inverse) puis /scale, et on teste
    // l'AABB du mesh charge. Suffisant pour selectionner en editeur.
    bool rayMesh(const ee::systems::Ray &_ray, const Vec3 &_center,
                 const Quat &_rot, const Vec3 &_scale, const ComponentInstance &_shape, float &_outT)
    {
        std::string path = stringOf(_shape.values, "path");
        const ee::core::CpuMesh &m = ee::core::getMesh(path);
        if (!m.valid)
            return false;

        Quat inv{_rot.w, -_rot.x, -_rot.y, -_rot.z};
        Vec3 ol = inv.rotate(_ray.origin - _center);
        Vec3 dl = inv.rotate(_ray.dir);
        Vec3 o{ol.x / _scale.x, ol.y / _scale.y, ol.z / _scale.z};
        Vec3 d{dl.x / _scale.x, dl.y / _scale.y, dl.z / _scale.z};

        float tx1 = (m.aabbMin.x - o.x) / d.x, tx2 = (m.aabbMax.x - o.x) / d.x;
        float ex = std::min(tx1, tx2), sx = std::max(tx1, tx2);
        float ty1 = (m.aabbMin.y - o.y) / d.y, ty2 = (m.aabbMax.y - o.y) / d.y;
        float ey = std::min(ty1, ty2), sy = std::max(ty1, ty2);
        float tz1 = (m.aabbMin.z - o.z) / d.z, tz2 = (m.aabbMax.z - o.z) / d.z;
        float ez = std::min(tz1, tz2), sz = std::max(tz1, tz2);

        float tEntree = std::max(ex, std::max(ey, ez));
        float tSortie = std::min(sx, std::min(sy, sz));
        if (tEntree > 0.0f && tEntree < tSortie)
        {
            _outT = tEntree;
            return true;
        }
        return false;
    }

    // --- rayon / capsule ---
    // Capsule = 2 spheres aux bouts + 1 cylindre fini. On teste les trois et on
    // garde la plus proche. radius scale par X, height par Y (comme le rendu).
    bool rayCapsule(const ee::systems::Ray &_ray, const Vec3 &_center,
                    const Quat &_rot, const Vec3 &_scale, const ComponentInstance &_shape, float &_outT)
    {
        float rad = valueOf(_shape.values, "radius", 0.5f) * _scale.x;
        float hgt = valueOf(_shape.values, "height", 1.0f) * _scale.y;

        Vec3 up = _rot.rotate({0.0f, 1.0f, 0.0f});
        Vec3 A = _center + up * (hgt * 0.5f);
        Vec3 B = _center - up * (hgt * 0.5f);

        const Vec3 &O = _ray.origin;
        const Vec3 &D = _ray.dir;

        float best = 1e30f;
        bool hit = false;

        float ts;
        if (sphereT(O, D, A, rad, ts) && ts < best)
        {
            best = ts;
            hit = true;
        }
        if (sphereT(O, D, B, rad, ts) && ts < best)
        {
            best = ts;
            hit = true;
        }

        // Cylindre fini d'axe A->B (intersection puis clamp sur le segment).
        Vec3 axis = B - A;
        float L = axis.Magnetude();
        if (L > 1e-6f)
        {
            axis = axis / L;
            Vec3 oa = O - A;
            Vec3 dPerp = D - axis * D.Dot(axis);
            Vec3 mPerp = oa - axis * oa.Dot(axis);
            float aa = dPerp.Dot(dPerp);
            float bb = 2.0f * mPerp.Dot(dPerp);
            float cc = mPerp.Dot(mPerp) - rad * rad;
            if (aa > 1e-9f)
            {
                float disc = bb * bb - 4.0f * aa * cc;
                if (disc > 0.0f)
                {
                    float tt = (-bb - std::sqrt(disc)) / (2.0f * aa);
                    float proj = (oa + D * tt).Dot(axis); // position le long de l'axe
                    if (tt > 0.0f && tt < best && proj >= 0.0f && proj <= L)
                    {
                        best = tt;
                        hit = true;
                    }
                }
            }
        }

        if (hit)
        {
            _outT = best;
            return true;
        }
        return false;
    }

    // -----------------------------------------------------------------------
    // LA TABLE DES FORMES : nom du composant -> comment le raycaster.
    // Pour ajouter une forme : ecrire sa fonction rayXxx ci-dessus, puis
    // ajouter UNE ligne ici. pickScene n'a pas a changer.
    // -----------------------------------------------------------------------
    const std::map<std::string, RaycastFn> &shapeRegistry()
    {
        static const std::map<std::string, RaycastFn> reg = {
            {"SphereComponent", &raySphere},
            {"RectComponent", &rayBox},
            {"CapsuleComponent", &rayCapsule},
            {"MeshComponent", &rayMesh},
        };
        return reg;
    }
}

namespace ee::systems
{
    int pickScene(const SceneInfo &_scene, const Ray &_ray)
    {
        int best = -1;
        float bestT = 1e30f; // "l'infini" : toute vraie distance sera plus petite

        for (int idx = 0; idx < static_cast<int>(_scene.entities.size()); ++idx)
        {
            const EntityInfo &ent = _scene.entities[idx];

            // Une forme a besoin de son Transform pour etre situee/orientee.
            const ComponentInstance *tf = nullptr;
            for (const auto &ci : ent.components)
                if (ci.name == "TransformComponent")
                    tf = &ci;
            if (!tf)
                continue;

            Vec3 center{valueOf(tf->values, "PositionX", 0.0f),
                        valueOf(tf->values, "PositionY", 0.0f),
                        valueOf(tf->values, "PositionZ", 0.0f)};

            Quat rot = Quat::fromEulerDeg(valueOf(tf->values, "EuleurX", 0.0f),
                                          valueOf(tf->values, "EuleurY", 0.0f),
                                          valueOf(tf->values, "EuleurZ", 0.0f));
            Vec3 scale{valueOf(tf->values, "ScaleX", 1.0f),
                       valueOf(tf->values, "ScaleY", 1.0f),
                       valueOf(tf->values, "ScaleZ", 1.0f)};

            // Chaque composant reconnu comme une forme est teste via la table.
            for (const auto &ci : ent.components)
            {
                auto it = shapeRegistry().find(ci.name);
                if (it == shapeRegistry().end())
                    continue;

                float tt = 0.0f;
                if (it->second(_ray, center, rot, scale, ci, tt) && tt > 0.0f && tt < bestT)
                {
                    bestT = tt;
                    best = idx;
                }
            }
        }
        return best;
    }
}
