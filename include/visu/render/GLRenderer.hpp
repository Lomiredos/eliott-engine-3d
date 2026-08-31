#pragma once

#include "visu/render/Renderer.hpp"

#include <vector>

// ---------------------------------------------------------------------------
// Backend OpenGL du contrat IRenderer.
//
// C'est le "A" extrait de l'ancien ScenePreview : shaders, mesh sphere, FBO,
// matrices, glDrawElements. Il rend dans un framebuffer hors-ecran et expose
// la texture couleur, a afficher via ImGui::Image dans l'editeur.
//
// Il ne connait NI la scene, NI les composants, NI l'ECS : on lui dit
// "dessine une sphere ici", pas "voici une entite".
// ---------------------------------------------------------------------------

namespace ee::render
{
    class GLRenderer : public IRenderer
    {
    public:
        ~GLRenderer() override;

        // --- Contrat IRenderer ---
        void beginScene(const Camera &cam, int width, int height) override;
        void endScene() override;
        MeshHandle builtin(Prim which) override;
        MeshHandle createMesh(const float *verts, int vertFloatCount,
                              const unsigned int *indices, int indexCount) override;
        void drawMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg, Color col) override;

        // Cible du rendu :
        //  - offscreen (defaut) : rend dans un FBO, texture() pour l'editeur (ImGui::Image).
        //  - onscreen : rend directement dans le framebuffer 0 (fenetre) -> le jeu.
        void setOffscreen(bool offscreen) { m_offscreen = offscreen; }

        unsigned int texture() const { return m_colorTex; } // 0 si echec
        void viewMatrix(float out[16]) const;               // column-major, 16 floats
        void projMatrix(float out[16]) const;

    private:
        bool ensureInit();            // charge GL + shader + meshes integres (une fois)
        void ensureFbo(int w, int h); // (re)cree le FBO si la taille change

        // Envoie des donnees CPU vers un VAO/VBO/EBO et renvoie un handle.
        MeshHandle uploadMesh(const float *verts, int vertFloatCount,
                              const unsigned int *idx, int idxCount);

        bool m_init = false;
        bool m_failed = false;
        bool m_offscreen = true; // true = FBO (editeur), false = fenetre (jeu)

        unsigned int m_program = 0;

        // Table de meshes : le handle expose est (index + 1), 0 = invalide.
        struct Mesh
        {
            unsigned int vao = 0, vbo = 0, ebo = 0;
            int indexCount = 0;
        };
        std::vector<Mesh> m_meshes;
        MeshHandle m_sphere = 0, m_cube = 0, m_cylinder = 0; // meshes integres

        int m_uMVP = -1, m_uModel = -1, m_uColor = -1, m_uGlow = -1;

        unsigned int m_fbo = 0, m_colorTex = 0, m_depthRbo = 0;
        int m_fboW = 0, m_fboH = 0;

        float m_view[16] = {0}; // memorises pour l'editeur (ImGuizmo)
        float m_proj[16] = {0};
        float m_vp[16] = {0}; // proj * view, reutilise a chaque drawSphere
    };
}
