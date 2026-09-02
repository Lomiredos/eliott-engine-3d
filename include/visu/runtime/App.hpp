#pragma once

// ---------------------------------------------------------------------------
// Hote runtime du jeu (opt-in, game-only) : le POINT D'ENTREE + la boucle.
//
// C'est la mécanique MOTEUR (fenetre, contexte GL, chargement scene->World,
// boucle input/update/flush/render). Le jeu n'ecrit plus rien de tout ca : il
// fournit seulement QUELLES briques il a, via deux callbacks (composants +
// systemes, tous deux générés par l'editeur). -> en utilisant visu, on ne
// touche jamais a main.
// ---------------------------------------------------------------------------

#include "ecs/World.hpp"

#include "visu/core/SceneInfo.hpp"
#include "visu/core/MeshStore.hpp"
#include "visu/render/GLRenderer.hpp"
#include "visu/input/Input.hpp"
#include "visu/scene/WorldLoader.hpp"
#include "visu/scene/SystemHost.hpp"
#include "visu/scene/WorldRender.hpp"

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ee::runtime
{
    struct Config
    {
        std::string projectRoot;             // scene.json, systems/, base des .obj
        std::string windowTitle = "Game";
        int width = 1000;
        int height = 700;
    };

    // Callbacks fournis par le jeu (générés par l'editeur).
    using RegisterComponentsFn = std::function<void(ee::scene::WorldRegistry &)>;
    using RegisterSystemsFn = std::function<void(ee::scene::SystemHost &)>;

    namespace detail
    {
        inline void pollInput(GLFWwindow *_win)
        {
            ee::input::InputState in;
            if (glfwGetKey(_win, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(_win, GLFW_KEY_Z) == GLFW_PRESS) in.moveZ -= 1.0f;
            if (glfwGetKey(_win, GLFW_KEY_S) == GLFW_PRESS) in.moveZ += 1.0f;
            if (glfwGetKey(_win, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(_win, GLFW_KEY_Q) == GLFW_PRESS) in.moveX -= 1.0f;
            if (glfwGetKey(_win, GLFW_KEY_D) == GLFW_PRESS) in.moveX += 1.0f;
            ee::input::state() = in;
        }
    }

    inline int run(const Config &_cfg,
                   const RegisterComponentsFn &_registerComponents,
                   const RegisterSystemsFn &_registerSystems)
    {
        // 1) Scene authoree.
        std::string scenePath = _cfg.projectRoot + "/assets/scene.json";
        std::optional<SceneInfo> scene = loadScene(scenePath);
        if (!scene)
        {
            std::fprintf(stderr, "[runtime] scene introuvable : %s\n", scenePath.c_str());
            return 1;
        }
        ee::core::setMeshBaseDir(_cfg.projectRoot);

        // 2) Composants connus (moteur + jeu).
        ee::scene::WorldRegistry reg;
        ee::scene::registerEngineComponents(reg);
        if (_registerComponents)
            _registerComponents(reg);

        ee::ecs::World world;

        // 3) Systemes AVANT le chargement (le flush les peuple).
        std::vector<SystemInfo> schedule =
            loadSystemsInDir(_cfg.projectRoot + "/systems");
        ee::scene::SystemHost host;
        if (_registerSystems)
            _registerSystems(host);
        host.build(world, reg, schedule);

        // 4) Scene -> World typé.
        std::vector<ee::ecs::EntityID> entities =
            ee::scene::loadSceneIntoWorld(world, *scene, reg);
        std::printf("[runtime] %zu entites, %zu systeme(s)\n", entities.size(), schedule.size());
        std::fflush(stdout);

        // 5) Fenetre + contexte OpenGL 3.3 core.
        if (!glfwInit())
        {
            std::fprintf(stderr, "[runtime] glfwInit a echoue\n");
            return 1;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        GLFWwindow *win = glfwCreateWindow(_cfg.width, _cfg.height,
                                           _cfg.windowTitle.c_str(), nullptr, nullptr);
        if (!win)
        {
            std::fprintf(stderr, "[runtime] creation fenetre a echoue\n");
            glfwTerminate();
            return 1;
        }
        glfwMakeContextCurrent(win);
        glfwSwapInterval(1);

        ee::render::GLRenderer renderer;
        renderer.setOffscreen(false);

        ee::render::Camera cam;
        cam.pos = {0.0f, 3.0f, 12.0f};
        cam.yaw = -1.5707963f;
        cam.pitch = -0.15f;

        // 6) Boucle.
        while (!glfwWindowShouldClose(win))
        {
            glfwPollEvents();
            if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(win, 1);

            static double last = glfwGetTime();
            double now = glfwGetTime();
            float dt = static_cast<float>(now - last);
            last = now;

            detail::pollInput(win);
            host.updateAll(world, dt);
            world.flush(); // commit des spawns/morts eventuels au runtime

            int w, h;
            glfwGetFramebufferSize(win, &w, &h);
            renderer.beginScene(cam, w, h);
            ee::scene::renderWorld(renderer, world, entities);
            renderer.endScene();

            glfwSwapBuffers(win);
        }

        glfwDestroyWindow(win);
        glfwTerminate();
        return 0;
    }
}
