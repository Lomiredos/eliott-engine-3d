#include "visu/render/GLRenderer.hpp"

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>
#include <string>
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

    // Sans "#version" : buildProgram ajoute la version et MAX_POINT_LIGHTS
    // (RenderSettings::maxPointLights, taille des tableaux de lampes).
    const char *kFrag = R"(
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
uniform vec3 uPointPos[MAX_POINT_LIGHTS];
uniform vec3 uPointColor[MAX_POINT_LIGHTS];
uniform float uPointRange[MAX_POINT_LIGHTS];

// Ombres. Soleil : shadow map orthographique. Lampes : atlas de profondeur,
// une ligne par lampe ombree, une case par face de cube (+X -X +Y -Y +Z -Z).
uniform int uSunShadowOn;
uniform sampler2DShadow uSunShadowMap;
uniform mat4 uSunVP;
uniform float uSunTexel;        // 1 / resolution
uniform float uSunNormalBias;   // decalage le long de la normale (monde)
uniform sampler2DShadow uPointShadowMap;
uniform int uPointShadowRow[MAX_POINT_LIGHTS]; // -1 = pas d'ombre
uniform vec2 uPointShadowTile;  // taille d'une case en uv de l'atlas
uniform float uPointShadowTexel; // 1 / resolution d'une case
uniform float uPointNear;
uniform int uTransparent; // 1 = blending premultiplie (verre...)
uniform float uOpacity;   // part du diffus gardee ; les reflets restent entiers
uniform int uFaceSide;    // 0 = toutes les faces, 1 = arriere seules, -1 = avant seules

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

// Lumiere recue d'une direction L (radiance deja attenuee), ajoutee a part
// en diffus et en reflet (un objet transparent n'attenue que le diffus).
void shade(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float metal, float rough,
           inout vec3 diff, inout vec3 spec)
{
    float NdL = max(dot(N, L), 0.0);
    if (NdL <= 0.0)
        return;
    vec3 H = normalize(V + L);
    float NdV = max(dot(N, V), 1e-4);
    float NdH = max(dot(N, H), 0.0);
    vec3 F0 = mix(vec3(0.04), albedo, metal);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
    vec3 s = distributionGGX(NdH, rough * rough) * geometrySmith(NdV, NdL, rough) * F
           / (4.0 * NdV * NdL + 1e-4);
    vec3 kd = (1.0 - F) * (1.0 - metal);
    diff += kd * albedo / PI * radiance * NdL;
    spec += s * radiance * NdL;
}

// Faces du cube d'une lampe : direction visee et "haut" (memes valeurs que
// kCubeFaces cote C++, qui fait le rendu de chaque case).
const vec3 kFaceF[6] = vec3[6](vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 1, 0),
                               vec3(0, -1, 0), vec3(0, 0, 1), vec3(0, 0, -1));
const vec3 kFaceU[6] = vec3[6](vec3(0, 1, 0), vec3(0, 1, 0), vec3(0, 0, 1),
                               vec3(0, 0, 1), vec3(0, 1, 0), vec3(0, 1, 0));

// 1 = eclaire, 0 = dans l'ombre du soleil. PCF 3x3 (chaque echantillon est
// deja filtre par le materiel), fondu vers 1 au bord de la zone couverte.
float sunShadow(vec3 P, vec3 Ng)
{
    vec4 lp = uSunVP * vec4(P + Ng * uSunNormalBias, 1.0);
    float edge = max(abs(lp.x), abs(lp.y));
    if (edge >= 1.0 || lp.z >= 1.0)
        return 1.0;
    vec3 c = lp.xyz * 0.5 + 0.5; // orthographique : w = 1
    float s = 0.0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            s += texture(uSunShadowMap, vec3(c.xy + vec2(x, y) * uSunTexel, c.z));
    return mix(s / 9.0, 1.0, smoothstep(0.85, 1.0, edge));
}

float pointShadow(int i, vec3 P, vec3 Ng)
{
    int row = uPointShadowRow[i];
    if (row < 0)
        return 1.0;
    vec3 d = P - uPointPos[i];
    // Decalage ~ taille d'un texel a cette distance (90 degres par face).
    d += Ng * (length(d) * 3.0 * uPointShadowTexel);
    vec3 a = abs(d);
    int f = (a.x >= a.y && a.x >= a.z) ? (d.x > 0.0 ? 0 : 1)
          : (a.y >= a.z)               ? (d.y > 0.0 ? 2 : 3)
                                       : (d.z > 0.0 ? 4 : 5);
    vec3 F = kFaceF[f];
    vec3 U = kFaceU[f];
    vec3 R = cross(F, U);
    float w = dot(d, F);
    vec2 uv = vec2(dot(d, R), dot(d, U)) / w * 0.5 + 0.5;
    // Profondeur telle que l'ecrit la projection perspective de la case.
    float n = uPointNear;
    float fa = max(uPointRange[i], n + 1e-3);
    float z = ((fa + n) / (fa - n) - 2.0 * fa * n / ((fa - n) * w)) * 0.5 + 0.5;

    float t = uPointShadowTexel;
    uv = clamp(uv, vec2(1.5 * t), vec2(1.0 - 1.5 * t)); // ne pas lire la case voisine
    vec2 base = vec2(float(f), float(row));
    float s = 0.0;
    for (int x = -1; x <= 1; x += 2)
        for (int y = -1; y <= 1; y += 2)
            s += texture(uPointShadowMap,
                         vec3((base + uv + vec2(x, y) * 0.5 * t) * uPointShadowTile, z));
    return s * 0.25;
}

// Tone mapping ACES (approx. Narkowicz).
vec3 aces(vec3 c)
{
    return clamp((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14), 0.0, 1.0);
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

    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 N = normalize(vNormal); // normales du mesh (pas de culling : sens des triangles libre)

    // Face avant/arriere d'apres la normale (le sens des triangles n'est pas
    // fiable). Un transparent est dessine en 2 fois : arriere puis avant.
    bool back = dot(N, V) < 0.0;
    if (uFaceSide != 0 && (back ? 1 : -1) != uFaceSide)
        discard;
    if (back && uTransparent != 0)
        N = -N; // face interieure, vue a travers l'objet
    vec3 Ng = N; // normale de la surface (sans normal map) : biais des ombres

    if ((uMapMask & 2) != 0)
    {
        vec3 T = normalize(vTangent.xyz - N * dot(N, vTangent.xyz));
        vec3 B = cross(N, T) * vTangent.w;
        vec3 tn = texture(uNormalMap, vUV).xyz * 2.0 - 1.0;
        tn.xy *= uNormalStrength;
        N = normalize(mat3(T, B, N) * tn);
    }

    // Ambiante : diffuse + reflet approche (pas de carte d'environnement ;
    // sans reflet, un metal, qui n'a pas de diffus, serait noir). Le reflet
    // prend aussi un peu du soleil, comme un ciel.
    float NdV = max(dot(N, V), 1e-4);
    vec3 F0 = mix(vec3(0.04), albedo, metal);
    vec3 Fenv = F0 + (max(vec3(1.0 - rough), F0) - F0) * pow(1.0 - NdV, 5.0);
    vec3 envLight = uAmbient + (uSunOn != 0 ? uSunColor * 0.2 : vec3(0.0));
    vec3 diff = uAmbient * (1.0 - Fenv) * (1.0 - metal) * albedo * ao;
    vec3 spec = envLight * Fenv * ao;
    if (uSunOn != 0)
    {
        vec3 L = normalize(-uSunDir);
        float sh = (uSunShadowOn != 0 && dot(Ng, L) > 0.0) ? sunShadow(vWorldPos, Ng) : 1.0;
        shade(N, V, L, uSunColor * sh, albedo, metal, rough, diff, spec);
    }
    for (int i = 0; i < uPointCount; ++i)
    {
        vec3 d = uPointPos[i] - vWorldPos;
        float dist = length(d);
        float r = max(uPointRange[i], 1e-3);
        float f = clamp(1.0 - pow(dist / r, 4.0), 0.0, 1.0);
        float att = f * f / (dist * dist + 1.0);
        if (att <= 0.0)
            continue;
        vec3 L = d / max(dist, 1e-4);
        float sh = dot(Ng, L) > 0.0 ? pointShadow(i, vWorldPos, Ng) : 1.0;
        shade(N, V, L, uPointColor[i] * att * sh, albedo, metal, rough, diff, spec);
    }

    vec3 emissive = toLinear(uEmissive) * uEmissiveIntensity;
    if ((uMapMask & 32) != 0)
        emissive *= texture(uEmissiveMap, vUV).rgb;

    if (uTransparent != 0)
    {
        // Le diffus est attenue, pas les reflets (une vitre reste brillante).
        // Plus on regarde en biais, plus elle reflechit et moins on voit au
        // travers (Fresnel) : l'alpha monte vers 1 sur les bords.
        float opacity = uOpacity;
        if ((uMapMask & 1) != 0)
            opacity *= texture(uAlbedoMap, vUV).a;
        float f = (Fenv.r + Fenv.g + Fenv.b) / 3.0;
        float alpha = clamp(opacity + (1.0 - opacity) * f, 0.0, 1.0);
        // Un reflet fort (soleil) cache ce qui est derriere.
        vec3 st = aces(spec);
        alpha = max(alpha, max(st.r, max(st.g, st.b)));
        alpha = max(alpha, 1e-3);

        // L'image est en sRGB 8 bits : le melange se fait en gamma. On sort
        // donc la couleur "droite" (apport / alpha) encodee, puis
        // premultipliee : additionner un apport lineaire deja encode
        // blanchirait tout le verre.
        vec3 straight = (diff * opacity + spec + emissive) / alpha;
        FragColor = vec4(pow(aces(straight), vec3(1.0 / 2.2)) * alpha, alpha);
        return;
    }

    vec3 color = diff + spec;

    color += emissive;

    // Tone mapping puis gamma.
    FragColor = vec4(pow(aces(color), vec3(1.0 / 2.2)), 1.0);
}
)";

    // Passe de profondeur des ombres : position seule, pas de couleur.
    const char *kDepthVert = R"(#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
)";
    const char *kDepthFrag = R"(#version 330 core
void main() {}
)";

    // Faces du cube d'ombre d'une lampe (direction, haut) : meme ordre et
    // memes vecteurs que kFaceF/kFaceU du shader.
    const float kCubeFaces[6][2][3] = {
        {{1, 0, 0}, {0, 1, 0}}, {{-1, 0, 0}, {0, 1, 0}}, {{0, 1, 0}, {0, 0, 1}},
        {{0, -1, 0}, {0, 0, 1}}, {{0, 0, 1}, {0, 1, 0}}, {{0, 0, -1}, {0, 1, 0}}};
    const float kPointShadowNear = 0.05f;
    const int kSunShadowUnit = 6;   // unites de texture apres les 6 maps
    const int kPointShadowUnit = 7;

    Mat4 ortho(float l, float r, float b, float t, float n, float f)
    {
        Mat4 m{};
        m.m[0] = 2.0f / (r - l);
        m.m[5] = 2.0f / (t - b);
        m.m[10] = -2.0f / (f - n);
        m.m[12] = -(r + l) / (r - l);
        m.m[13] = -(t + b) / (t - b);
        m.m[14] = -(f + n) / (f - n);
        m.m[15] = 1.0f;
        return m;
    }


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

    bool GLRenderer::buildProgram()
    {
        const int maxPoints = m_settings.maxPointLights;
        std::string frag = "#version 330 core\n#define MAX_POINT_LIGHTS " +
                           std::to_string(maxPoints) + "\n" + kFrag;
        unsigned int vs = compile(GL_VERTEX_SHADER, kVert);
        unsigned int fs = compile(GL_FRAGMENT_SHADER, frag.c_str());
        unsigned int prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);
        glDeleteShader(vs);
        glDeleteShader(fs);
        int ok = 0;
        glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            char log[1024];
            glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
            std::fprintf(stderr, "[GLRenderer] link du shader (%d lampes) : %s\n", maxPoints, log);
            glDeleteProgram(prog);
            return false;
        }
        if (m_program != 0)
            glDeleteProgram(m_program);
        m_program = prog;
        m_programMaxPoints = maxPoints;

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
        m_uTransparent = glGetUniformLocation(m_program, "uTransparent");
        m_uOpacity = glGetUniformLocation(m_program, "uOpacity");
        m_uFaceSide = glGetUniformLocation(m_program, "uFaceSide");
        m_uSunShadowOn = glGetUniformLocation(m_program, "uSunShadowOn");
        m_uSunVP = glGetUniformLocation(m_program, "uSunVP");
        m_uSunTexel = glGetUniformLocation(m_program, "uSunTexel");
        m_uSunNormalBias = glGetUniformLocation(m_program, "uSunNormalBias");
        m_uPointShadowRow = glGetUniformLocation(m_program, "uPointShadowRow");
        m_uPointShadowTile = glGetUniformLocation(m_program, "uPointShadowTile");
        m_uPointShadowTexel = glGetUniformLocation(m_program, "uPointShadowTexel");
        m_uPointNear = glGetUniformLocation(m_program, "uPointNear");

        // Une unite de texture fixe par map (ordre de MapSlot), puis les ombres.
        glUseProgram(m_program);
        const char *samplers[] = {"uAlbedoMap", "uNormalMap", "uMetallicMap",
                                  "uRoughnessMap", "uAOMap", "uEmissiveMap"};
        for (int i = 0; i < static_cast<int>(MapSlot::Count); ++i)
            glUniform1i(glGetUniformLocation(m_program, samplers[i]), i);
        glUniform1i(glGetUniformLocation(m_program, "uSunShadowMap"), kSunShadowUnit);
        glUniform1i(glGetUniformLocation(m_program, "uPointShadowMap"), kPointShadowUnit);
        return true;
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

        // --- Programmes ---
        if (!buildProgram())
        {
            m_failed = true;
            return false;
        }
        {
            unsigned int vs = compile(GL_VERTEX_SHADER, kDepthVert);
            unsigned int fs = compile(GL_FRAGMENT_SHADER, kDepthFrag);
            m_depthProgram = glCreateProgram();
            glAttachShader(m_depthProgram, vs);
            glAttachShader(m_depthProgram, fs);
            glLinkProgram(m_depthProgram);
            glDeleteShader(vs);
            glDeleteShader(fs);
            m_uDepthMVP = glGetUniformLocation(m_depthProgram, "uMVP");
        }

        // --- Shadow maps (1x1 tant qu'aucune ombre n'est demandee : les
        // samplers des unites 6 et 7 ont toujours une texture valide) ---
        auto makeShadowTarget = [](unsigned int &_fbo, unsigned int &_tex, bool _border)
        {
            glGenFramebuffers(1, &_fbo);
            glGenTextures(1, &_tex);
            glBindTexture(GL_TEXTURE_2D, _tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1, 1, 0, GL_DEPTH_COMPONENT,
                         GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            if (_border) // hors de la zone : profondeur max = eclaire
            {
                const float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, one);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _tex, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        };
        makeShadowTarget(m_sunFbo, m_sunTex, true);
        makeShadowTarget(m_pointFbo, m_pointTex, false);
        m_sunRes = 1;
        m_pointTile = 1;
        m_pointRows = 1;

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
        flushView(); // image precedente pas terminee par endScene
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
        m_frameFbo = m_offscreen ? m_targets[m_target].fbo : 0;
        glBindFramebuffer(GL_FRAMEBUFFER, m_frameFbo);
        glViewport(0, 0, width, height);
        glDisable(GL_SCISSOR_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GLRenderer::beginView(const Camera &cam, const Viewport &v)
    {
        flushView(); // draw de la vue precedente (avec ses matrices)
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
        m_viewGL[0] = v.x;
        m_viewGL[1] = glY;
        m_viewGL[2] = v.w;
        m_viewGL[3] = v.h;
        glEnable(GL_SCISSOR_TEST); // l'effacement ne deborde pas de la vue
        glScissor(v.x, glY, v.w, v.h);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);

        for (int i = 0; i < 3; ++i)
        {
            m_camPos[i] = eye[i];
            m_camFwd[i] = fwd[i];
        }
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
        m_lights = lights; // envoyees au shader par flushView
    }

    void GLRenderer::setSettings(const RenderSettings &settings)
    {
        RenderSettings s = settings;
        auto clampi = [](int _v, int _lo, int _hi) { return std::max(_lo, std::min(_hi, _v)); };
        s.maxPointLights = clampi(s.maxPointLights, 1, 64);
        s.sunShadowResolution = clampi(s.sunShadowResolution, 512, 8192);
        s.sunShadowDistance = std::max(1.0f, std::min(1000.0f, s.sunShadowDistance));
        s.maxShadowedPointLights = clampi(s.maxShadowedPointLights, 0, 16);
        s.pointShadowResolution = clampi(s.pointShadowResolution, 128, 2048);
        if (s == m_settings)
            return;
        // Le shader est recompile au prochain flushView si le nombre de lampes
        // change ; les shadow maps sont reallouees a leur prochain usage.
        m_settings = s;
    }

    // Les maxPointLights lampes qui comptent le plus pour la camera : celles
    // dont la zone est la plus proche (distance - portee ; dedans = negatif).
    // Les premieres avec shadows recoivent une ligne de l'atlas d'ombres.
    void GLRenderer::selectPointLights()
    {
        std::vector<std::pair<float, int>> order;
        for (int i = 0; i < static_cast<int>(m_lights.points.size()); ++i)
        {
            const PointLight &p = m_lights.points[i];
            float dx = p.pos.x - m_camPos[0], dy = p.pos.y - m_camPos[1], dz = p.pos.z - m_camPos[2];
            order.push_back({std::sqrt(dx * dx + dy * dy + dz * dz) - p.range, i});
        }
        std::stable_sort(order.begin(), order.end(),
                         [](const auto &_a, const auto &_b) { return _a.first < _b.first; });

        m_activePoints.clear();
        m_activeRows.clear();
        int shadowed = 0;
        const int maxShadowed = m_settings.pointShadows ? m_settings.maxShadowedPointLights : 0;
        for (const auto &o : order)
        {
            if (static_cast<int>(m_activePoints.size()) >= m_settings.maxPointLights)
                break;
            const PointLight &p = m_lights.points[o.second];
            m_activePoints.push_back(p);
            m_activeRows.push_back(p.shadows && shadowed < maxShadowed ? shadowed++ : -1);
        }
    }

    void GLRenderer::applyLights(bool sunShadow, const float sunVP[16], float sunTexelWorld)
    {
        float c[3];
        linearColor(m_lights.ambient, m_lights.ambientIntensity, c);
        glUniform3fv(m_uAmbient, 1, c);
        glUniform1i(m_uSunOn, m_lights.sun ? 1 : 0);
        float d[3] = {m_lights.sunDir.x, m_lights.sunDir.y, m_lights.sunDir.z};
        glUniform3fv(m_uSunDir, 1, d);
        linearColor(m_lights.sunColor, m_lights.sunIntensity, c);
        glUniform3fv(m_uSunColor, 1, c);

        glUniform1i(m_uSunShadowOn, sunShadow ? 1 : 0);
        if (sunShadow)
        {
            glUniformMatrix4fv(m_uSunVP, 1, GL_FALSE, sunVP);
            glUniform1f(m_uSunTexel, 1.0f / static_cast<float>(m_sunRes));
            glUniform1f(m_uSunNormalBias, sunTexelWorld * 1.5f);
        }

        const int count = static_cast<int>(m_activePoints.size());
        std::vector<float> pos(count * 3), col(count * 3), range(count);
        for (int i = 0; i < count; ++i)
        {
            const PointLight &p = m_activePoints[i];
            pos[i * 3] = p.pos.x;
            pos[i * 3 + 1] = p.pos.y;
            pos[i * 3 + 2] = p.pos.z;
            linearColor(p.color, p.intensity, &col[i * 3]);
            range[i] = p.range;
        }
        glUniform1i(m_uPointCount, count);
        if (count > 0)
        {
            glUniform3fv(m_uPointPos, count, pos.data());
            glUniform3fv(m_uPointColor, count, col.data());
            glUniform1fv(m_uPointRange, count, range.data());
            glUniform1iv(m_uPointShadowRow, count, m_activeRows.data());
        }
        glUniform2f(m_uPointShadowTile, 1.0f / 6.0f, 1.0f / static_cast<float>(m_pointRows));
        glUniform1f(m_uPointShadowTexel, 1.0f / static_cast<float>(m_pointTile));
        glUniform1f(m_uPointNear, kPointShadowNear);
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
        float r2 = 0.0f;
        for (int i = 0; i + 2 < vertFloatCount; i += kVertexFloats)
            r2 = std::max(r2, verts[i] * verts[i] + verts[i + 1] * verts[i + 1] +
                                  verts[i + 2] * verts[i + 2]);
        e.radius = std::sqrt(r2);
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

    // Matrice model = translate * rotation * scale, centre et rayon monde.
    static void queuedTransform(Vec3 pos, Vec3 scale, Vec3 eulerDeg, float meshRadius,
                                float outModel[16], float outPos[3], float &outRadius)
    {
        Mat4 rot = rotationMat(eulerDeg.x, eulerDeg.y, eulerDeg.z);
        Mat4 model = mul(translate(pos.x, pos.y, pos.z),
                         mul(rot, scale3(scale.x, scale.y, scale.z)));
        for (int i = 0; i < 16; ++i)
            outModel[i] = model.m[i];
        outPos[0] = pos.x;
        outPos[1] = pos.y;
        outPos[2] = pos.z;
        float sx = std::fabs(scale.x), sy = std::fabs(scale.y), sz = std::fabs(scale.z);
        outRadius = meshRadius * std::max(sx, std::max(sy, sz));
    }

    void GLRenderer::drawMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg, const Material &mat)
    {
        if (!m_init || mesh == 0 || mesh > m_meshes.size())
            return;
        Queued q;
        q.kind = mat.transparent ? DrawKind::Transparent : DrawKind::Opaque;
        q.mesh = mesh;
        q.mat = mat;
        queuedTransform(pos, scale, eulerDeg, m_meshes[mesh - 1].radius, q.model, q.pos, q.radius);
        float dx = pos.x - m_camPos[0], dy = pos.y - m_camPos[1], dz = pos.z - m_camPos[2];
        q.dist2 = dx * dx + dy * dy + dz * dz;
        m_queue.push_back(q);
    }

    void GLRenderer::drawWireMesh(MeshHandle mesh, Vec3 pos, Vec3 scale, Vec3 eulerDeg, Color col)
    {
        if (!m_init || mesh == 0 || mesh > m_meshes.size())
            return;
        Queued q;
        q.kind = DrawKind::Wire;
        q.mesh = mesh;
        q.mat.albedo = col;
        queuedTransform(pos, scale, eulerDeg, m_meshes[mesh - 1].radius, q.model, q.pos, q.radius);
        m_queue.push_back(q);
    }

    void GLRenderer::ensureShadowTargets()
    {
        int maxTex = 4096;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTex);

        int sunRes = std::min(m_settings.sunShadowResolution, maxTex);
        if (m_settings.sunShadows && sunRes != m_sunRes)
        {
            glBindTexture(GL_TEXTURE_2D, m_sunTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, sunRes, sunRes, 0,
                         GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            m_sunRes = sunRes;
        }

        // Atlas : 6 cases de large, une ligne par lampe ombree ; on reduit
        // les cases si l'atlas depasse la taille max d'une texture.
        int rows = std::max(1, m_settings.maxShadowedPointLights);
        int tile = m_settings.pointShadowResolution;
        while (tile > 16 && (tile * 6 > maxTex || tile * rows > maxTex))
            tile /= 2;
        if (m_settings.pointShadows && m_settings.maxShadowedPointLights > 0 &&
            (tile != m_pointTile || rows != m_pointRows))
        {
            glBindTexture(GL_TEXTURE_2D, m_pointTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, tile * 6, tile * rows, 0,
                         GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            m_pointTile = tile;
            m_pointRows = rows;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GLRenderer::drawDepth(const Queued &q, const float vp[16])
    {
        const Mesh &m = m_meshes[q.mesh - 1];
        Mat4 a, b;
        for (int i = 0; i < 16; ++i)
        {
            a.m[i] = vp[i];
            b.m[i] = q.model[i];
        }
        Mat4 mvp = mul(a, b);
        glUniformMatrix4fv(m_uDepthMVP, 1, GL_FALSE, mvp.m);
        glBindVertexArray(m.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
        glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    // Shadow map du soleil : boite orthographique couvrant sunShadowDistance
    // devant la camera, vue depuis le soleil. Le centre est cale sur la
    // grille des texels pour que l'ombre ne scintille pas quand on bouge.
    void GLRenderer::renderSunShadow(float outVP[16])
    {
        const float D = m_settings.sunShadowDistance;
        const float h = D * 0.5f;
        float dir[3] = {m_lights.sunDir.x, m_lights.sunDir.y, m_lights.sunDir.z};
        normalize3(dir);
        float up[3] = {0.0f, 1.0f, 0.0f};
        if (std::fabs(dir[1]) > 0.99f)
        {
            up[0] = 1.0f;
            up[1] = 0.0f;
        }
        const float origin[3] = {0.0f, 0.0f, 0.0f};
        Mat4 view = lookAt(origin, dir, up); // rotation seule

        float ctr[3] = {m_camPos[0] + m_camFwd[0] * h, m_camPos[1] + m_camFwd[1] * h,
                        m_camPos[2] + m_camFwd[2] * h};
        float cx = view.m[0] * ctr[0] + view.m[4] * ctr[1] + view.m[8] * ctr[2];
        float cy = view.m[1] * ctr[0] + view.m[5] * ctr[1] + view.m[9] * ctr[2];
        float cz = view.m[2] * ctr[0] + view.m[6] * ctr[1] + view.m[10] * ctr[2];
        const float texel = 2.0f * h / static_cast<float>(m_sunRes);
        cx = std::floor(cx / texel) * texel;
        cy = std::floor(cy / texel) * texel;
        // z vue negatif devant : near/far autour du centre, large en
        // profondeur pour garder les objets hors champ qui projettent.
        Mat4 proj = ortho(cx - h, cx + h, cy - h, cy + h, -cz - 2.0f * D, -cz + 2.0f * D);
        Mat4 vp = mul(proj, view);
        for (int i = 0; i < 16; ++i)
            outVP[i] = vp.m[i];

        glBindFramebuffer(GL_FRAMEBUFFER, m_sunFbo);
        glViewport(0, 0, m_sunRes, m_sunRes);
        glClear(GL_DEPTH_BUFFER_BIT);
        for (const Queued &q : m_queue)
            if (q.kind == DrawKind::Opaque)
                drawDepth(q, outVP);
    }

    // Lampes ombrees : la scene vue depuis la lampe dans les 6 directions,
    // chaque face dans sa case de l'atlas (colonne = face, ligne = lampe).
    void GLRenderer::renderPointShadows(const std::vector<int> &rows)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pointFbo);
        glViewport(0, 0, m_pointTile * 6, m_pointTile * m_pointRows);
        glClear(GL_DEPTH_BUFFER_BIT);
        for (size_t li = 0; li < m_activePoints.size(); ++li)
        {
            int row = rows[li];
            if (row < 0)
                continue;
            const PointLight &p = m_activePoints[li];
            float eye[3] = {p.pos.x, p.pos.y, p.pos.z};
            float range = std::max(p.range, kPointShadowNear * 2.0f);
            Mat4 proj = perspective(3.14159265f * 0.5f, 1.0f, kPointShadowNear, range);
            for (int f = 0; f < 6; ++f)
            {
                const float *F = kCubeFaces[f][0];
                float ctr[3] = {eye[0] + F[0], eye[1] + F[1], eye[2] + F[2]};
                Mat4 vp = mul(proj, lookAt(eye, ctr, kCubeFaces[f][1]));
                glViewport(f * m_pointTile, row * m_pointTile, m_pointTile, m_pointTile);
                for (const Queued &q : m_queue)
                {
                    if (q.kind != DrawKind::Opaque)
                        continue;
                    float dx = q.pos[0] - eye[0], dy = q.pos[1] - eye[1], dz = q.pos[2] - eye[2];
                    float reach = range + q.radius;
                    if (dx * dx + dy * dy + dz * dz > reach * reach)
                        continue; // trop loin pour etre eclaire par cette lampe
                    drawDepth(q, vp.m);
                }
            }
        }
    }

    // Fin d'une vue : ombres, puis opaques, fil de fer, et transparents du
    // plus loin au plus proche.
    void GLRenderer::flushView()
    {
        if (m_queue.empty())
            return;
        if (!m_init || !m_frameOk)
        {
            m_queue.clear();
            return;
        }
        if (m_programMaxPoints != m_settings.maxPointLights && !buildProgram())
        {
            // Echec (trop de lampes pour le GPU ?) : on garde l'ancien shader.
            m_settings.maxPointLights = m_programMaxPoints;
        }

        selectPointLights();
        ensureShadowTargets();

        bool anyOpaque = false;
        for (const Queued &q : m_queue)
            anyOpaque |= q.kind == DrawKind::Opaque;
        const bool sunShadow = anyOpaque && m_lights.sun && m_lights.sunShadows &&
                               m_settings.sunShadows;
        bool pointShadow = false;
        for (int r : m_activeRows)
            pointShadow |= r >= 0;
        pointShadow &= anyOpaque;
        if (!pointShadow)
            std::fill(m_activeRows.begin(), m_activeRows.end(), -1);

        // --- Passes de profondeur ---
        float sunVP[16] = {};
        if (sunShadow || pointShadow)
        {
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_CLAMP); // casters derriere le plan proche : gardes
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(2.0f, 4.0f); // contre l'acne d'ombre
            glUseProgram(m_depthProgram);
            if (sunShadow)
                renderSunShadow(sunVP);
            if (pointShadow)
                renderPointShadows(m_activeRows);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glDisable(GL_DEPTH_CLAMP);

            glBindFramebuffer(GL_FRAMEBUFFER, m_frameFbo);
            glViewport(m_viewGL[0], m_viewGL[1], m_viewGL[2], m_viewGL[3]);
        }

        // --- Passe principale ---
        glUseProgram(m_program);
        glUniform3fv(m_uCamPos, 1, m_camPos);
        applyLights(sunShadow, sunVP, 2.0f * m_settings.sunShadowDistance * 0.5f /
                                          static_cast<float>(std::max(1, m_sunRes)));
        glActiveTexture(GL_TEXTURE0 + kSunShadowUnit);
        glBindTexture(GL_TEXTURE_2D, m_sunTex);
        glActiveTexture(GL_TEXTURE0 + kPointShadowUnit);
        glBindTexture(GL_TEXTURE_2D, m_pointTex);
        glActiveTexture(GL_TEXTURE0);

        for (const Queued &q : m_queue)
            if (q.kind == DrawKind::Opaque)
                drawMeshNow(q.mesh, q.model, q.mat, 0);
        for (const Queued &q : m_queue)
            if (q.kind == DrawKind::Wire)
                drawWireNow(q.mesh, q.model, q.mat.albedo);

        // Transparents : chacun se melange a ce qui est deja dessine derriere.
        // Pas d'ecriture de profondeur, sinon un transparent cacherait les
        // suivants ; le test reste actif (caches par les opaques).
        std::vector<const Queued *> tr;
        for (const Queued &q : m_queue)
            if (q.kind == DrawKind::Transparent)
                tr.push_back(&q);
        if (!tr.empty())
        {
            std::stable_sort(tr.begin(), tr.end(), [](const Queued *_a, const Queued *_b)
                             { return _a->dist2 > _b->dist2; });
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // couleur premultipliee
            glDepthMask(GL_FALSE);
            for (const Queued *q : tr)
            {
                drawMeshNow(q->mesh, q->model, q->mat, 1);  // faces arriere (interieur)
                drawMeshNow(q->mesh, q->model, q->mat, -1); // puis faces avant
            }
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
        m_queue.clear();
    }

    void GLRenderer::drawMeshNow(MeshHandle mesh, const float modelM[16], const Material &mat,
                                 int faceSide)
    {
        const Mesh &m = m_meshes[mesh - 1];
        Mat4 vp, model;
        for (int i = 0; i < 16; ++i)
        {
            vp.m[i] = m_vp[i];
            model.m[i] = modelM[i];
        }
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
        glUniform1i(m_uTransparent, mat.transparent ? 1 : 0);
        glUniform1f(m_uOpacity, mat.opacity);
        glUniform1i(m_uFaceSide, faceSide);

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

    void GLRenderer::drawWireNow(MeshHandle mesh, const float modelM[16], Color col)
    {
        const Mesh &m = m_meshes[mesh - 1];
        Mat4 vp, model;
        for (int i = 0; i < 16; ++i)
        {
            vp.m[i] = m_vp[i];
            model.m[i] = modelM[i];
        }
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
        flushView();
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
