#include "visu/render/GLRenderer.hpp"

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>
#include <vector>

#include "math/Quaternion.hpp"

// ---------------------------------------------------------------------------
// Petites matrices 4x4 en column-major (convention OpenGL) + shaders.
// (Repris tel quel de l'ancien ScenePreview : c'est la partie "comment on
//  dessine", elle est ici a sa place, cachee derriere le contrat.)
// ---------------------------------------------------------------------------
namespace
{
    struct Mat4
    {
        float m[16];
    };

    Mat4 mul(const Mat4 &a, const Mat4 &b)
    {
        Mat4 r{};
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
            {
                float s = 0.0f;
                for (int k = 0; k < 4; ++k)
                    s += a.m[k * 4 + row] * b.m[col * 4 + k];
                r.m[col * 4 + row] = s;
            }
        return r;
    }

    Mat4 translate(float x, float y, float z)
    {
        Mat4 r{};
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        r.m[12] = x;
        r.m[13] = y;
        r.m[14] = z;
        return r;
    }

    Mat4 scale3(float x, float y, float z)
    {
        Mat4 r{};
        r.m[0] = x;
        r.m[5] = y;
        r.m[10] = z;
        r.m[15] = 1.0f;
        return r;
    }

    // Matrice de rotation depuis les angles d'Euler (degres), via le meme
    // quaternion que le picking -> draw et pick s'accordent forcement.
    Mat4 rotationMat(float rxDeg, float ryDeg, float rzDeg)
    {
        ee::math::Quaternion q = ee::math::Quaternion::fromEulerDeg(rxDeg, ryDeg, rzDeg);
        float w = q.w, x = q.x, y = q.y, z = q.z;
        Mat4 r{};
        r.m[0] = 1 - 2 * (y * y + z * z);
        r.m[1] = 2 * (x * y + w * z);
        r.m[2] = 2 * (x * z - w * y);
        r.m[4] = 2 * (x * y - w * z);
        r.m[5] = 1 - 2 * (x * x + z * z);
        r.m[6] = 2 * (y * z + w * x);
        r.m[8] = 2 * (x * z + w * y);
        r.m[9] = 2 * (y * z - w * x);
        r.m[10] = 1 - 2 * (x * x + y * y);
        r.m[15] = 1.0f;
        return r;
    }

    Mat4 perspective(float fovyRad, float aspect, float n, float f)
    {
        Mat4 r{};
        float t = std::tan(fovyRad * 0.5f);
        r.m[0] = 1.0f / (aspect * t);
        r.m[5] = 1.0f / t;
        r.m[10] = -(f + n) / (f - n);
        r.m[11] = -1.0f;
        r.m[14] = -(2.0f * f * n) / (f - n);
        return r;
    }

    void normalize3(float v[3])
    {
        float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (l > 0.0f)
        {
            v[0] /= l;
            v[1] /= l;
            v[2] /= l;
        }
    }

    void forwardFrom(float yaw, float pitch, float out[3])
    {
        float cp = std::cos(pitch), sp = std::sin(pitch);
        float cy = std::cos(yaw), sy = std::sin(yaw);
        out[0] = cp * cy;
        out[1] = sp;
        out[2] = cp * sy;
    }

    Mat4 lookAt(const float eye[3], const float ctr[3], const float up[3])
    {
        float f[3] = {ctr[0] - eye[0], ctr[1] - eye[1], ctr[2] - eye[2]};
        normalize3(f);
        float s[3] = {f[1] * up[2] - f[2] * up[1],
                      f[2] * up[0] - f[0] * up[2],
                      f[0] * up[1] - f[1] * up[0]};
        normalize3(s);
        float u[3] = {s[1] * f[2] - s[2] * f[1],
                      s[2] * f[0] - s[0] * f[2],
                      s[0] * f[1] - s[1] * f[0]};

        Mat4 r{};
        r.m[0] = s[0];
        r.m[4] = s[1];
        r.m[8] = s[2];
        r.m[1] = u[0];
        r.m[5] = u[1];
        r.m[9] = u[2];
        r.m[2] = -f[0];
        r.m[6] = -f[1];
        r.m[10] = -f[2];
        r.m[12] = -(s[0] * eye[0] + s[1] * eye[1] + s[2] * eye[2]);
        r.m[13] = -(u[0] * eye[0] + u[1] * eye[1] + u[2] * eye[2]);
        r.m[14] = (f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2]);
        r.m[15] = 1.0f;
        return r;
    }

    const char *kVert = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 uMVP;
uniform mat4 uModel;
out vec3 vNormal;
void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
    vNormal = mat3(uModel) * aNormal;
}
)";

    const char *kFrag = R"(#version 330 core
in vec3 vNormal;
uniform vec3 uColor;
uniform float uGlow; // 0 = ombrage normal, >0 = couleur plate avec alpha=uGlow (glow)
out vec4 FragColor;
void main()
{
    if (uGlow > 0.0)
    {
        FragColor = vec4(uColor, uGlow);
        return;
    }
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vec3(0.5, 1.0, 0.3));
    float d = max(dot(N, L), 0.0) * 0.8 + 0.2;
    FragColor = vec4(uColor * d, 1.0);
}
)";

    unsigned int compile(unsigned int type, const char *src)
    {
        unsigned int s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        int ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char log[512];
            glGetShaderInfoLog(s, 512, nullptr, log);
            std::fprintf(stderr, "[GLRenderer] shader: %s\n", log);
        }
        return s;
    }
}

// ---------------------------------------------------------------------------

namespace ee::render
{
    GLRenderer::~GLRenderer()
    {
        // Les ressources GL sont liberees par le contexte a la fermeture.
    }

    bool GLRenderer::ensureInit()
    {
        if (m_init)
            return true;
        if (m_failed)
            return false;

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::fprintf(stderr, "[GLRenderer] gladLoadGLLoader a echoue\n");
            m_failed = true;
            return false;
        }

        // --- Programme ---
        unsigned int vs = compile(GL_VERTEX_SHADER, kVert);
        unsigned int fs = compile(GL_FRAGMENT_SHADER, kFrag);
        m_program = glCreateProgram();
        glAttachShader(m_program, vs);
        glAttachShader(m_program, fs);
        glLinkProgram(m_program);
        glDeleteShader(vs);
        glDeleteShader(fs);
        m_uMVP = glGetUniformLocation(m_program, "uMVP");
        m_uModel = glGetUniformLocation(m_program, "uModel");
        m_uColor = glGetUniformLocation(m_program, "uColor");
        m_uGlow = glGetUniformLocation(m_program, "uGlow");

        // --- Meshes integres : sphere, cube, cylindre (unitaires) ---
        const float PI = 3.14159265358979323846f;

        // Sphere UV unitaire (position == normale).
        {
            const int stacks = 16, sectors = 24;
            std::vector<float> v; // x,y,z, nx,ny,nz
            std::vector<unsigned int> ix;
            for (int i = 0; i <= stacks; ++i)
            {
                float phi = PI * 0.5f - PI * (float)i / stacks; // +90 -> -90
                float y = std::sin(phi), r = std::cos(phi);
                for (int j = 0; j <= sectors; ++j)
                {
                    float theta = 2.0f * PI * (float)j / sectors;
                    float x = r * std::cos(theta), z = r * std::sin(theta);
                    v.insert(v.end(), {x, y, z, x, y, z});
                }
            }
            for (int i = 0; i < stacks; ++i)
                for (int j = 0; j < sectors; ++j)
                {
                    unsigned int a = i * (sectors + 1) + j;
                    unsigned int b = a + sectors + 1;
                    ix.insert(ix.end(), {a, b, a + 1, a + 1, b, b + 1});
                }
            m_sphere = uploadMesh(v.data(), (int)v.size(), ix.data(), (int)ix.size());

            // Aretes seules : anneaux de latitude + meridiens, jamais les
            // diagonales des quads (contrairement a glPolygonMode(GL_LINE)).
            std::vector<unsigned int> wireIx;
            for (int i = 0; i <= stacks; ++i)
                for (int j = 0; j < sectors; ++j)
                {
                    unsigned int a = i * (sectors + 1) + j;
                    wireIx.insert(wireIx.end(), {a, a + 1});
                }
            for (int j = 0; j <= sectors; ++j)
                for (int i = 0; i < stacks; ++i)
                {
                    unsigned int a = i * (sectors + 1) + j;
                    wireIx.insert(wireIx.end(), {a, a + sectors + 1});
                }
            attachWireIndices(m_sphere, wireIx.data(), (int)wireIx.size());
        }

        // Cube sur [-0.5, 0.5]^3, une normale PAR FACE (faces bien plates).
        {
            const float hs = 0.5f;
            struct Face
            {
                float n[3];
                float v[4][3];
            };
            const Face faces[6] = {
                {{0, 0, 1}, {{-hs, -hs, hs}, {hs, -hs, hs}, {hs, hs, hs}, {-hs, hs, hs}}},      // +Z
                {{0, 0, -1}, {{hs, -hs, -hs}, {-hs, -hs, -hs}, {-hs, hs, -hs}, {hs, hs, -hs}}}, // -Z
                {{1, 0, 0}, {{hs, -hs, hs}, {hs, -hs, -hs}, {hs, hs, -hs}, {hs, hs, hs}}},      // +X
                {{-1, 0, 0}, {{-hs, -hs, -hs}, {-hs, -hs, hs}, {-hs, hs, hs}, {-hs, hs, -hs}}}, // -X
                {{0, 1, 0}, {{-hs, hs, hs}, {hs, hs, hs}, {hs, hs, -hs}, {-hs, hs, -hs}}},      // +Y
                {{0, -1, 0}, {{-hs, -hs, -hs}, {hs, -hs, -hs}, {hs, -hs, hs}, {-hs, -hs, hs}}}, // -Y
            };
            std::vector<float> v;
            std::vector<unsigned int> ix;
            for (int f = 0; f < 6; ++f)
            {
                unsigned int base = f * 4;
                for (int k = 0; k < 4; ++k)
                    v.insert(v.end(), {faces[f].v[k][0], faces[f].v[k][1], faces[f].v[k][2],
                                       faces[f].n[0], faces[f].n[1], faces[f].n[2]});
                ix.insert(ix.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
            }
            m_cube = uploadMesh(v.data(), (int)v.size(), ix.data(), (int)ix.size());

            // Aretes seules : le perimetre de chaque face, pas la diagonale
            // qui coupe le quad en 2 triangles.
            std::vector<unsigned int> wireIx;
            for (int f = 0; f < 6; ++f)
            {
                unsigned int base = f * 4;
                wireIx.insert(wireIx.end(), {base, base + 1, base + 1, base + 2,
                                             base + 2, base + 3, base + 3, base});
            }
            attachWireIndices(m_cube, wireIx.data(), (int)wireIx.size());
        }

        // Cylindre unitaire : rayon 1, hauteur 1 (y de -0.5 a 0.5) + 2 disques.
        {
            const int sectors = 24;
            const float hh = 0.5f;
            std::vector<float> v;
            std::vector<unsigned int> ix;

            // Cote : normale radiale (cos, 0, sin), 2 sommets par secteur.
            for (int j = 0; j <= sectors; ++j)
            {
                float theta = 2.0f * PI * (float)j / sectors;
                float cx = std::cos(theta), cz = std::sin(theta);
                v.insert(v.end(), {cx, -hh, cz, cx, 0.0f, cz});
                v.insert(v.end(), {cx, hh, cz, cx, 0.0f, cz});
            }
            for (int j = 0; j < sectors; ++j)
            {
                unsigned int a = j * 2;
                ix.insert(ix.end(), {a, a + 2, a + 1, a + 1, a + 2, a + 3});
            }

            // Disques haut (+Y) et bas (-Y) : un centre + un anneau.
            auto addCap = [&](float y, float ny)
            {
                unsigned int center = (unsigned int)(v.size() / 6);
                v.insert(v.end(), {0.0f, y, 0.0f, 0.0f, ny, 0.0f});
                unsigned int first = (unsigned int)(v.size() / 6);
                for (int j = 0; j <= sectors; ++j)
                {
                    float theta = 2.0f * PI * (float)j / sectors;
                    v.insert(v.end(), {std::cos(theta), y, std::sin(theta), 0.0f, ny, 0.0f});
                }
                for (int j = 0; j < sectors; ++j)
                {
                    unsigned int a = first + j, b = first + j + 1;
                    if (ny > 0.0f)
                        ix.insert(ix.end(), {center, a, b});
                    else
                        ix.insert(ix.end(), {center, b, a});
                }
            };
            addCap(hh, 1.0f);
            addCap(-hh, -1.0f);

            m_cylinder = uploadMesh(v.data(), (int)v.size(), ix.data(), (int)ix.size());

            // Aretes seules : les 2 anneaux (haut/bas) + les verticales,
            // jamais la diagonale qui coupe chaque quad lateral.
            std::vector<unsigned int> wireIx;
            for (int j = 0; j < sectors; ++j)
            {
                unsigned int a = (unsigned int)j * 2, b = (unsigned int)(j + 1) * 2;
                wireIx.insert(wireIx.end(), {a, b, a + 1, b + 1, a, a + 1});
            }
            attachWireIndices(m_cylinder, wireIx.data(), (int)wireIx.size());
        }

        // Demi-sphere unitaire, bombee vers +Y (dome) : utilisee pour les
        // bouts d'une capsule. Meme construction que la sphere complete,
        // mais phi ne va que de +90 a 0 (reste au-dessus de l'equateur).
        {
            const int stacks = 8, sectors = 24;
            std::vector<float> v;
            std::vector<unsigned int> ix;
            for (int i = 0; i <= stacks; ++i)
            {
                float phi = PI * 0.5f * (1.0f - (float)i / stacks); // +90 -> 0
                float y = std::sin(phi), r = std::cos(phi);
                for (int j = 0; j <= sectors; ++j)
                {
                    float theta = 2.0f * PI * (float)j / sectors;
                    float x = r * std::cos(theta), z = r * std::sin(theta);
                    v.insert(v.end(), {x, y, z, x, y, z});
                }
            }
            for (int i = 0; i < stacks; ++i)
                for (int j = 0; j < sectors; ++j)
                {
                    unsigned int a = i * (sectors + 1) + j;
                    unsigned int b = a + sectors + 1;
                    ix.insert(ix.end(), {a, b, a + 1, a + 1, b, b + 1});
                }
            m_hemisphere = uploadMesh(v.data(), (int)v.size(), ix.data(), (int)ix.size());

            std::vector<unsigned int> wireIx;
            for (int i = 0; i <= stacks; ++i)
                for (int j = 0; j < sectors; ++j)
                {
                    unsigned int a = i * (sectors + 1) + j;
                    wireIx.insert(wireIx.end(), {a, a + 1});
                }
            for (int j = 0; j <= sectors; ++j)
                for (int i = 0; i < stacks; ++i)
                {
                    unsigned int a = i * (sectors + 1) + j;
                    wireIx.insert(wireIx.end(), {a, a + sectors + 1});
                }
            attachWireIndices(m_hemisphere, wireIx.data(), (int)wireIx.size());
        }

        m_init = true;
        return true;
    }

    void GLRenderer::ensureFbo(int w, int h)
    {
        if (m_fbo != 0 && w == m_fboW && h == m_fboH)
            return;

        if (m_fbo == 0)
        {
            glGenFramebuffers(1, &m_fbo);
            glGenTextures(1, &m_colorTex);
            glGenRenderbuffers(1, &m_depthRbo);
        }

        m_fboW = w;
        m_fboH = h;

        glBindTexture(GL_TEXTURE_2D, m_colorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);

        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLRenderer::beginScene(const Camera &cam, int width, int height)
    {
        if (width <= 0 || height <= 0)
            return;
        if (!ensureInit())
            return;

        if (m_offscreen)
            ensureFbo(width, height);

        // Matrices derivees de la camera-donnee.
        float eye[3] = {cam.pos.x, cam.pos.y, cam.pos.z};
        float fwd[3];
        forwardFrom(cam.yaw, cam.pitch, fwd);
        float ctr[3] = {eye[0] + fwd[0], eye[1] + fwd[1], eye[2] + fwd[2]};
        float up[3] = {0.0f, 1.0f, 0.0f};
        Mat4 proj = perspective(cam.fovDeg * 3.14159265f / 180.0f,
                                (float)width / (float)height, cam.nearZ, cam.farZ);
        Mat4 view = lookAt(eye, ctr, up);
        Mat4 vp = mul(proj, view);
        for (int i = 0; i < 16; ++i)
        {
            m_view[i] = view.m[i];
            m_proj[i] = proj.m[i];
            m_vp[i] = vp.m[i];
        }

        // Onscreen : framebuffer 0 (la fenetre). Offscreen : notre FBO texture.
        glBindFramebuffer(GL_FRAMEBUFFER, m_offscreen ? m_fbo : 0);
        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_program);
        glUniform1f(m_uGlow, 0.0f); // chaque drawMesh bindera son propre VAO
    }

    MeshHandle GLRenderer::uploadMesh(const float *verts, int vertFloatCount,
                                      const unsigned int *idx, int idxCount)
    {
        Mesh e;
        e.indexCount = idxCount;
        glGenVertexArrays(1, &e.vao);
        glGenBuffers(1, &e.vbo);
        glGenBuffers(1, &e.ebo);
        glBindVertexArray(e.vao);
        glBindBuffer(GL_ARRAY_BUFFER, e.vbo);
        glBufferData(GL_ARRAY_BUFFER, (long long)(vertFloatCount * sizeof(float)), verts, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, e.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (long long)(idxCount * sizeof(unsigned int)), idx, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
        glBindVertexArray(0);
        m_meshes.push_back(e);
        return (MeshHandle)m_meshes.size(); // handle = index + 1
    }

    void GLRenderer::attachWireIndices(MeshHandle mesh, const unsigned int *idx, int idxCount)
    {
        if (mesh == 0 || mesh > m_meshes.size())
            return;
        Mesh &m = m_meshes[mesh - 1];

        glBindVertexArray(m.vao);
        glGenBuffers(1, &m.wireEbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.wireEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (long long)(idxCount * sizeof(unsigned int)), idx, GL_STATIC_DRAW);
        glBindVertexArray(0);
        m.wireIndexCount = idxCount;
    }

    MeshHandle GLRenderer::builtin(Prim which)
    {
        if (!ensureInit())
            return 0;
        switch (which)
        {
        case Prim::Sphere:
            return m_sphere;
        case Prim::Cube:
            return m_cube;
        case Prim::Cylinder:
            return m_cylinder;
        case Prim::Hemisphere:
            return m_hemisphere;
        }
        return 0;
    }

    MeshHandle GLRenderer::createMesh(const float *verts, int vertFloatCount,
                                      const unsigned int *indices, int indexCount)
    {
        if (!ensureInit())
            return 0;
        if (!verts || vertFloatCount <= 0 || !indices || indexCount <= 0)
            return 0;
        return uploadMesh(verts, vertFloatCount, indices, indexCount);
    }

    void GLRenderer::drawMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg, Color col)
    {
        if (!m_init || mesh == 0 || mesh > m_meshes.size())
            return;
        const Mesh &m = m_meshes[mesh - 1];

        Mat4 vp;
        for (int i = 0; i < 16; ++i)
            vp.m[i] = m_vp[i];
        // model = translate * rotation * scale : on scale, on oriente, on place.
        Mat4 rot = rotationMat(eulerDeg.x, eulerDeg.y, eulerDeg.z);
        Mat4 model = mul(translate(pos.x, pos.y, pos.z),
                         mul(rot, scale3(scale.x, scale.y, scale.z)));
        Mat4 mvp = mul(vp, model);

        glBindVertexArray(m.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo); // attachWireIndices a pu changer l'EBO du VAO
        glUniform1f(m_uGlow, 0.0f);
        glUniform3f(m_uColor, col.r, col.g, col.b);
        glUniformMatrix4fv(m_uMVP, 1, GL_FALSE, mvp.m);
        glUniformMatrix4fv(m_uModel, 1, GL_FALSE, model.m);
        glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    void GLRenderer::drawWireMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg, Color col)
    {
        if (!m_init || mesh == 0 || mesh > m_meshes.size())
            return;
        const Mesh &m = m_meshes[mesh - 1];

        Mat4 vp;
        for (int i = 0; i < 16; ++i)
            vp.m[i] = m_vp[i];
        Mat4 rot = rotationMat(eulerDeg.x, eulerDeg.y, eulerDeg.z);
        Mat4 model = mul(translate(pos.x, pos.y, pos.z),
                         mul(rot, scale3(scale.x, scale.y, scale.z)));
        Mat4 mvp = mul(vp, model);

        glBindVertexArray(m.vao);
        glUniform1f(m_uGlow, 1.0f); // couleur plate, pas d'ombrage sur des aretes
        glUniform3f(m_uColor, col.r, col.g, col.b);
        glUniformMatrix4fv(m_uMVP, 1, GL_FALSE, mvp.m);
        glUniformMatrix4fv(m_uModel, 1, GL_FALSE, model.m);

        if (m.wireEbo != 0)
        {
            // Vrai jeu d'aretes (anneaux/perimetres) : aucune diagonale de
            // triangulation n'est dessinee.
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.wireEbo);
            glDrawElements(GL_LINES, m.wireIndexCount, GL_UNSIGNED_INT, nullptr);
        }
        else
        {
            // Mesh custom (ex. import MeshComponent) sans variante dediee :
            // repli sur le mode GL_LINE, qui lui dessine les diagonales.
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, nullptr);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    void GLRenderer::endScene()
    {
        if (!m_init)
            return;
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // IMPORTANT : rendre la main a ImGui
    }

    void GLRenderer::viewMatrix(float out[16]) const
    {
        for (int i = 0; i < 16; ++i)
            out[i] = m_view[i];
    }

    void GLRenderer::projMatrix(float out[16]) const
    {
        for (int i = 0; i < 16; ++i)
            out[i] = m_proj[i];
    }
}
