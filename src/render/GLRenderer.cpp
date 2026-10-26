#include "visu/render/GLRenderer.hpp"

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>
#include <vector>

#include "math/Quaternion.hpp"
#include "visu/render/MeshUtil.hpp"

#include <algorithm>
#include <stb_image.h>

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

    // Shader standard : PBR metal/rugosite (Cook-Torrance GGX), soleil +
    // lumieres ponctuelles + ambiante, puis tone mapping ACES et gamma.
    // uGlow > 0 : couleur plate (contours fil de fer, surbrillance).
    const char *kVert = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in vec4 aTangent;
uniform mat4 uMVP;
uniform mat4 uModel;
uniform vec4 uUVTransform; // xy = repetition, zw = decalage
out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec4 vTangent;
void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
    vWorldPos = (uModel * vec4(aPos, 1.0)).xyz;
    mat3 normalMat = transpose(inverse(mat3(uModel))); // echelle non uniforme
    vNormal = normalMat * aNormal;
    vTangent = vec4(mat3(uModel) * aTangent.xyz, aTangent.w);
    vUV = aUV * uUVTransform.xy + uUVTransform.zw;
}
)";

    const char *kFrag = R"(#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec4 vTangent;

uniform vec3 uColor;      // albedo (sRGB)
uniform float uGlow;      // 0 = eclaire, >0 = couleur plate avec alpha=uGlow
uniform float uMetallic;
uniform float uRoughness;
uniform float uAO;
uniform vec3 uEmissive;   // sRGB
uniform float uEmissiveIntensity;
uniform float uNormalStrength;
uniform int uMapMask;     // bit i = map MapSlot i presente
uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uMetallicMap;
uniform sampler2D uRoughnessMap;
uniform sampler2D uAOMap;
uniform sampler2D uEmissiveMap;

uniform vec3 uCamPos;
uniform vec3 uAmbient;    // lineaire, intensite comprise
uniform int uSunOn;
uniform vec3 uSunDir;     // direction de la lumiere (vers la scene)
uniform vec3 uSunColor;   // lineaire, intensite comprise
uniform int uPointCount;
uniform vec3 uPointPos[8];
uniform vec3 uPointColor[8];
uniform float uPointRange[8];

out vec4 FragColor;
const float PI = 3.14159265;

vec3 toLinear(vec3 c) { return pow(c, vec3(2.2)); }

float distributionGGX(float NdH, float a)
{
    float a2 = a * a;
    float d = NdH * NdH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float geometrySmith(float NdV, float NdL, float rough)
{
    float k = (rough + 1.0) * (rough + 1.0) / 8.0;
    return (NdV / (NdV * (1.0 - k) + k)) * (NdL / (NdL * (1.0 - k) + k));
}

// Lumiere recue d'une direction L (radiance deja attenuee).
vec3 shade(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float metal, float rough)
{
    float NdL = max(dot(N, L), 0.0);
    if (NdL <= 0.0)
        return vec3(0.0);
    vec3 H = normalize(V + L);
    float NdV = max(dot(N, V), 1e-4);
    float NdH = max(dot(N, H), 0.0);
    vec3 F0 = mix(vec3(0.04), albedo, metal);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
    vec3 spec = distributionGGX(NdH, rough * rough) * geometrySmith(NdV, NdL, rough) * F
              / (4.0 * NdV * NdL + 1e-4);
    vec3 kd = (1.0 - F) * (1.0 - metal);
    return (kd * albedo / PI + spec) * radiance * NdL;
}

void main()
{
    if (uGlow > 0.0)
    {
        FragColor = vec4(uColor, uGlow);
        return;
    }

    vec3 albedo = toLinear(uColor);
    if ((uMapMask & 1) != 0)
        albedo *= texture(uAlbedoMap, vUV).rgb; // texture sRGB : deja lineaire
    float metal = uMetallic;
    if ((uMapMask & 4) != 0)
        metal *= texture(uMetallicMap, vUV).r;
    float rough = uRoughness;
    if ((uMapMask & 8) != 0)
        rough *= texture(uRoughnessMap, vUV).r;
    rough = clamp(rough, 0.04, 1.0);
    float ao = uAO;
    if ((uMapMask & 16) != 0)
        ao *= texture(uAOMap, vUV).r;

    vec3 N = normalize(vNormal); // normales du mesh (pas de culling : sens des triangles libre)
    if ((uMapMask & 2) != 0)
    {
        vec3 T = normalize(vTangent.xyz - N * dot(N, vTangent.xyz));
        vec3 B = cross(N, T) * vTangent.w;
        vec3 tn = texture(uNormalMap, vUV).xyz * 2.0 - 1.0;
        tn.xy *= uNormalStrength;
        N = normalize(mat3(T, B, N) * tn);
    }
    vec3 V = normalize(uCamPos - vWorldPos);

    // Ambiante : diffuse + reflet approche (pas de carte d'environnement ;
    // sans reflet, un metal, qui n'a pas de diffus, serait noir). Le reflet
    // prend aussi un peu du soleil, comme un ciel.
    float NdV = max(dot(N, V), 1e-4);
    vec3 F0 = mix(vec3(0.04), albedo, metal);
    vec3 Fenv = F0 + (max(vec3(1.0 - rough), F0) - F0) * pow(1.0 - NdV, 5.0);
    vec3 envLight = uAmbient + (uSunOn != 0 ? uSunColor * 0.2 : vec3(0.0));
    vec3 color = (uAmbient * (1.0 - Fenv) * (1.0 - metal) * albedo + envLight * Fenv) * ao;
    if (uSunOn != 0)
        color += shade(N, V, normalize(-uSunDir), uSunColor, albedo, metal, rough);
    for (int i = 0; i < uPointCount; ++i)
    {
        vec3 d = uPointPos[i] - vWorldPos;
        float dist = length(d);
        float r = max(uPointRange[i], 1e-3);
        float f = clamp(1.0 - pow(dist / r, 4.0), 0.0, 1.0);
        float att = f * f / (dist * dist + 1.0);
        color += shade(N, V, d / max(dist, 1e-4), uPointColor[i] * att, albedo, metal, rough);
    }

    vec3 emissive = toLinear(uEmissive) * uEmissiveIntensity;
    if ((uMapMask & 32) != 0)
        emissive *= texture(uEmissiveMap, vUV).rgb;
    color += emissive;

    // Tone mapping ACES (approx. Narkowicz) puis gamma.
    color = clamp((color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14), 0.0, 1.0);
    FragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
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
        m_uMetallic = glGetUniformLocation(m_program, "uMetallic");
        m_uRoughness = glGetUniformLocation(m_program, "uRoughness");
        m_uAO = glGetUniformLocation(m_program, "uAO");
        m_uEmissive = glGetUniformLocation(m_program, "uEmissive");
        m_uEmissiveIntensity = glGetUniformLocation(m_program, "uEmissiveIntensity");
        m_uNormalStrength = glGetUniformLocation(m_program, "uNormalStrength");
        m_uUVTransform = glGetUniformLocation(m_program, "uUVTransform");
        m_uMapMask = glGetUniformLocation(m_program, "uMapMask");
        m_uCamPos = glGetUniformLocation(m_program, "uCamPos");
        m_uAmbient = glGetUniformLocation(m_program, "uAmbient");
        m_uSunOn = glGetUniformLocation(m_program, "uSunOn");
        m_uSunDir = glGetUniformLocation(m_program, "uSunDir");
        m_uSunColor = glGetUniformLocation(m_program, "uSunColor");
        m_uPointCount = glGetUniformLocation(m_program, "uPointCount");
        m_uPointPos = glGetUniformLocation(m_program, "uPointPos");
        m_uPointColor = glGetUniformLocation(m_program, "uPointColor");
        m_uPointRange = glGetUniformLocation(m_program, "uPointRange");

        // Une unite de texture fixe par map (ordre de MapSlot).
        glUseProgram(m_program);
        const char *samplers[] = {"uAlbedoMap", "uNormalMap", "uMetallicMap",
                                  "uRoughnessMap", "uAOMap", "uEmissiveMap"};
        for (int i = 0; i < static_cast<int>(MapSlot::Count); ++i)
            glUniform1i(glGetUniformLocation(m_program, samplers[i]), i);

        // --- Meshes integres : sphere, cube, cylindre (unitaires) ---
        const float PI = 3.14159265358979323846f;

        // Sphere UV unitaire (position == normale).
        {
            const int stacks = 16, sectors = 24;
            std::vector<float> v; // x,y,z, nx,ny,nz, u,v
            std::vector<unsigned int> ix;
            for (int i = 0; i <= stacks; ++i)
            {
                float phi = PI * 0.5f - PI * (float)i / stacks; // +90 -> -90
                float y = std::sin(phi), r = std::cos(phi);
                for (int j = 0; j <= sectors; ++j)
                {
                    float theta = 2.0f * PI * (float)j / sectors;
                    float x = r * std::cos(theta), z = r * std::sin(theta);
                    v.insert(v.end(), {x, y, z, x, y, z,
                                       1.0f - (float)j / sectors, 1.0f - (float)i / stacks});
                }
            }
            for (int i = 0; i < stacks; ++i)
                for (int j = 0; j < sectors; ++j)
                {
                    unsigned int a = i * (sectors + 1) + j;
                    unsigned int b = a + sectors + 1;
                    ix.insert(ix.end(), {a, b, a + 1, a + 1, b, b + 1});
                }
            v = withTangents(v, ix);
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
            const float uv[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}}; // texture entiere par face
            for (int f = 0; f < 6; ++f)
            {
                unsigned int base = f * 4;
                for (int k = 0; k < 4; ++k)
                    v.insert(v.end(), {faces[f].v[k][0], faces[f].v[k][1], faces[f].v[k][2],
                                       faces[f].n[0], faces[f].n[1], faces[f].n[2], uv[k][0], uv[k][1]});
                ix.insert(ix.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
            }
            v = withTangents(v, ix);
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
                float u = 1.0f - (float)j / sectors;
                v.insert(v.end(), {cx, -hh, cz, cx, 0.0f, cz, u, 0.0f});
                v.insert(v.end(), {cx, hh, cz, cx, 0.0f, cz, u, 1.0f});
            }
            for (int j = 0; j < sectors; ++j)
            {
                unsigned int a = j * 2;
                ix.insert(ix.end(), {a, a + 2, a + 1, a + 1, a + 2, a + 3});
            }

            // Disques haut (+Y) et bas (-Y) : un centre + un anneau.
            auto addCap = [&](float y, float ny)
            {
                unsigned int center = (unsigned int)(v.size() / 8);
                v.insert(v.end(), {0.0f, y, 0.0f, 0.0f, ny, 0.0f, 0.5f, 0.5f});
                unsigned int first = (unsigned int)(v.size() / 8);
                for (int j = 0; j <= sectors; ++j)
                {
                    float theta = 2.0f * PI * (float)j / sectors;
                    float c = std::cos(theta), sn = std::sin(theta);
                    v.insert(v.end(), {c, y, sn, 0.0f, ny, 0.0f, 0.5f + 0.5f * c, 0.5f + 0.5f * sn * ny});
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

            v = withTangents(v, ix);
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
                    v.insert(v.end(), {x, y, z, x, y, z,
                                       1.0f - (float)j / sectors, 1.0f - 0.5f * (float)i / stacks});
                }
            }
            for (int i = 0; i < stacks; ++i)
                for (int j = 0; j < sectors; ++j)
                {
                    unsigned int a = i * (sectors + 1) + j;
                    unsigned int b = a + sectors + 1;
                    ix.insert(ix.end(), {a, b, a + 1, a + 1, b, b + 1});
                }
            v = withTangents(v, ix);
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

    void GLRenderer::setTarget(int index)
    {
        if (index < 0)
            index = 0;
        if (index >= static_cast<int>(m_targets.size()))
            m_targets.resize(index + 1);
        m_target = index;
    }

    void GLRenderer::ensureFbo(int w, int h)
    {
        Target &t = m_targets[m_target];
        if (t.fbo != 0 && w == t.w && h == t.h)
            return;

        if (t.fbo == 0)
        {
            glGenFramebuffers(1, &t.fbo);
            glGenTextures(1, &t.colorTex);
            glGenRenderbuffers(1, &t.depthRbo);
        }

        t.w = w;
        t.h = h;

        glBindTexture(GL_TEXTURE_2D, t.colorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindRenderbuffer(GL_RENDERBUFFER, t.depthRbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);

        glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.colorTex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, t.depthRbo);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLRenderer::beginScene(const Camera &cam, int width, int height)
    {
        beginFrame(width, height);
        beginView(cam, Viewport{0, 0, width, height});
    }

    void GLRenderer::beginFrame(int width, int height)
    {
        m_frameOk = false;
        if (width <= 0 || height <= 0)
            return;
        if (!ensureInit())
            return;

        if (m_offscreen)
            ensureFbo(width, height);
        m_frameW = width;
        m_frameH = height;
        m_frameOk = true;

        // Onscreen : framebuffer 0 (la fenetre). Offscreen : notre FBO texture.
        // Fond noir : ce qui reste hors des vues (cases vides de l'ecran partage).
        glBindFramebuffer(GL_FRAMEBUFFER, m_offscreen ? m_targets[m_target].fbo : 0);
        glViewport(0, 0, width, height);
        glDisable(GL_SCISSOR_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GLRenderer::beginView(const Camera &cam, const Viewport &v)
    {
        if (!m_frameOk || v.w <= 0 || v.h <= 0)
            return;

        // Matrices derivees de la camera-donnee (aspect = celui de la vue).
        float eye[3] = {cam.pos.x, cam.pos.y, cam.pos.z};
        float fwd[3];
        forwardFrom(cam.yaw, cam.pitch, fwd);
        float ctr[3] = {eye[0] + fwd[0], eye[1] + fwd[1], eye[2] + fwd[2]};
        float up[3] = {0.0f, 1.0f, 0.0f};
        Mat4 proj = perspective(cam.fovDeg * 3.14159265f / 180.0f,
                                (float)v.w / (float)v.h, cam.nearZ, cam.farZ);
        Mat4 view = lookAt(eye, ctr, up);
        Mat4 vp = mul(proj, view);
        for (int i = 0; i < 16; ++i)
        {
            m_view[i] = view.m[i];
            m_proj[i] = proj.m[i];
            m_vp[i] = vp.m[i];
        }

        // GL compte y depuis le bas ; Viewport depuis le haut.
        int glY = m_frameH - v.y - v.h;
        glViewport(v.x, glY, v.w, v.h);
        glEnable(GL_SCISSOR_TEST); // l'effacement ne deborde pas de la vue
        glScissor(v.x, glY, v.w, v.h);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);

        glUseProgram(m_program);
        glUniform1f(m_uGlow, 0.0f); // chaque drawMesh bindera son propre VAO
        m_camPos[0] = eye[0];
        m_camPos[1] = eye[1];
        m_camPos[2] = eye[2];
        glUniform3fv(m_uCamPos, 1, m_camPos);
        applyLights();
    }

    // Couleur sRGB * intensite -> lineaire (ce qu'attend le shader).
    static void linearColor(const Color &_c, float _intensity, float _out[3])
    {
        _out[0] = std::pow(_c.r, 2.2f) * _intensity;
        _out[1] = std::pow(_c.g, 2.2f) * _intensity;
        _out[2] = std::pow(_c.b, 2.2f) * _intensity;
    }

    void GLRenderer::setLights(const Lights &lights)
    {
        m_lights = lights;
        if (m_init)
        {
            glUseProgram(m_program);
            applyLights();
        }
    }

    void GLRenderer::applyLights()
    {
        float c[3];
        linearColor(m_lights.ambient, m_lights.ambientIntensity, c);
        glUniform3fv(m_uAmbient, 1, c);
        glUniform1i(m_uSunOn, m_lights.sun ? 1 : 0);
        float d[3] = {m_lights.sunDir.x, m_lights.sunDir.y, m_lights.sunDir.z};
        glUniform3fv(m_uSunDir, 1, d);
        linearColor(m_lights.sunColor, m_lights.sunIntensity, c);
        glUniform3fv(m_uSunColor, 1, c);

        const int count = std::min(static_cast<int>(m_lights.points.size()), kMaxPointLights);
        float pos[kMaxPointLights * 3], col[kMaxPointLights * 3], range[kMaxPointLights];
        for (int i = 0; i < count; ++i)
        {
            const PointLight &p = m_lights.points[i];
            pos[i * 3] = p.pos.x;
            pos[i * 3 + 1] = p.pos.y;
            pos[i * 3 + 2] = p.pos.z;
            linearColor(p.color, p.intensity, &col[i * 3]);
            range[i] = p.range;
        }
        glUniform1i(m_uPointCount, count);
        if (count > 0)
        {
            glUniform3fv(m_uPointPos, count, pos);
            glUniform3fv(m_uPointColor, count, col);
            glUniform1fv(m_uPointRange, count, range);
        }
    }

    TextureHandle GLRenderer::loadTexture(const std::string &path, bool srgb)
    {
        if (path.empty() || !ensureInit())
            return 0;
        namespace fs = std::filesystem;
        const std::string key = (srgb ? "s|" : "l|") + path;
        CachedTexture &ct = m_textures[key];

        // Fichier modifie (re-export) : recharge, verifie au plus 1 fois/s.
        auto now = std::chrono::steady_clock::now();
        bool fresh = ct.lastCheck.time_since_epoch().count() == 0;
        if (!fresh && now - ct.lastCheck < std::chrono::seconds(1))
            return ct.tex;
        ct.lastCheck = now;
        std::error_code ec;
        fs::file_time_type mtime = fs::last_write_time(path, ec);
        if (ec)
            return ct.tex; // fichier absent : on garde l'ancienne (ou 0)
        if (!fresh && ct.tex != 0 && mtime == ct.mtime)
            return ct.tex;
        ct.mtime = mtime;

        int w = 0, h = 0, n = 0;
        stbi_set_flip_vertically_on_load(1); // uv (0,0) = bas gauche, comme GL
        unsigned char *px = stbi_load(path.c_str(), &w, &h, &n, 4);
        if (!px)
        {
            std::fprintf(stderr, "[GLRenderer] texture illisible : %s\n", path.c_str());
            return ct.tex;
        }
        if (ct.tex == 0)
            glGenTextures(1, &ct.tex);
        glBindTexture(GL_TEXTURE_2D, ct.tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, px);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(px);
        return ct.tex;
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
        // Format kVertexFloats : position, normale, uv, tangente (MeshUtil.hpp).
        const int stride = kVertexFloats * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void *)(8 * sizeof(float)));
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

    void GLRenderer::drawMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg, const Material &mat)
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
        glUniform3f(m_uColor, mat.albedo.r, mat.albedo.g, mat.albedo.b);
        glUniform1f(m_uMetallic, mat.metallic);
        glUniform1f(m_uRoughness, mat.roughness);
        glUniform1f(m_uAO, mat.ao);
        glUniform3f(m_uEmissive, mat.emissive.r, mat.emissive.g, mat.emissive.b);
        glUniform1f(m_uEmissiveIntensity, mat.emissiveIntensity);
        glUniform1f(m_uNormalStrength, mat.normalStrength);
        glUniform4f(m_uUVTransform, mat.tiling[0], mat.tiling[1], mat.offset[0], mat.offset[1]);

        int mask = 0;
        for (int i = 0; i < static_cast<int>(MapSlot::Count); ++i)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, mat.maps[i]);
            if (mat.maps[i] != 0)
                mask |= 1 << i;
        }
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(m_uMapMask, mask);

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
