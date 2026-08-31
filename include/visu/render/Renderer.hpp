#pragma once

// ---------------------------------------------------------------------------
// Le CONTRAT de rendu.
//
// IRenderer decrit *ce que* le jeu sait montrer (des verbes de haut niveau),
// jamais *comment* c'est dessine. Aucune techno ici : ni gl..., ni raylib.
// Les systemes du jeu ne parlent qu'a cette interface ; un backend concret
// (GLRenderer aujourd'hui, un RaylibRenderer demain) l'implemente.
//
// C'est le renversement de dependance : le gameplay depend de l'abstraction,
// les technos vivent en dessous.
// ---------------------------------------------------------------------------

namespace ee::render
{
    struct Vec3
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
    };

    struct Color
    {
        float r = 1.0f, g = 1.0f, b = 1.0f;
    };

    struct Camera
    {
        Vec3 pos;
        float yaw = 0.0f;
        float pitch = 0.0f;
        float fovDeg = 45.0f;
        float nearZ = 0.1f;
        float farZ = 200.0f;
    };

    // Reference opaque vers un mesh cote backend (un id, jamais un type GL/raylib).
    // 0 = handle invalide.
    using MeshHandle = unsigned int;

    // Meshes integres, generes une fois par le backend (unitaires, centres).
    enum class Prim
    {
        Sphere,  // rayon 1
        Cube,    // [-0.5, 0.5]^3
        Cylinder // rayon 1, hauteur 1 (y de -0.5 a 0.5)
    };

    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        virtual void beginScene(const Camera &cam, int width, int height) = 0;
        virtual void endScene() = 0;

        // Handle d'un mesh integre. Le contrat ne grossit plus par forme : une
        // forme nouvelle = un mesh de plus, pas une methode de plus.
        virtual MeshHandle builtin(Prim which) = 0;

        // Cree un mesh depuis des donnees CPU interleaved (x,y,z, nx,ny,nz).
        // vertFloatCount = nombre de floats ; indexCount = nombre d'indices.
        virtual MeshHandle createMesh(const float *verts, int vertFloatCount,
                                      const unsigned int *indices, int indexCount) = 0;

        // Dessine un mesh a (pos, echelle par axe, rotation en degres X/Y/Z).
        virtual void drawMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg, Color col) = 0;
    };
}
