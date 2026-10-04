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
#include "visu/scene/WorldLoader.hpp"
#include "visu/scene/SystemHost.hpp"
#include "visu/scene/WorldRender.hpp"
#include "visu/scene/EngineSystems.hpp"

#include "input/InputManager.hpp"
#include "input/Keys.hpp"

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ee::runtime
{
    struct Config
    {
        // Assets/ScenesDatas/, systems/, base des .obj. Laisse vide (defaut) :
        // deduit automatiquement de l'emplacement de l'executable (convention
        // <racine>/build/<exe>, celle qu'ecrit eliott-hub) -> marche quel que
        // soit le dossier courant au lancement (terminal, double-clic, Play
        // depuis l'editeur qui se place dans le dossier de l'exe...).
        std::string projectRoot;
        std::string sceneName = "BaseScene";  // Assets/ScenesDatas/<sceneName>.json
        std::string windowTitle = "Game";
        int width = 1000;
        int height = 700;
    };

    // Callbacks fournis par le jeu (générés par l'editeur).
    using RegisterComponentsFn = std::function<void(ee::scene::WorldRegistry &)>;
    using RegisterSystemsFn = std::function<void(ee::scene::SystemHost &)>;

    namespace detail
    {
        // Racine du projet deduite du chemin de l'executable (convention
        // <racine>/build/<exe>), independante du dossier courant.
        inline std::string defaultProjectRoot()
        {
#ifdef _WIN32
            char buffer[MAX_PATH];
            GetModuleFileNameA(nullptr, buffer, MAX_PATH);
            std::filesystem::path exe(buffer);
#else
            std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe");
#endif
            return exe.parent_path().parent_path().string(); // .../build/exe -> ...
        }

        // Table GLFW <-> ee::input::Key, dans le MEME ORDRE que l'enum Key
        // (meme convention que s_keyTable cote SDL dans eliott-input).
        // La taille est deduite du tableau -> jamais de constante a resynchroniser
        // a la main si Keys.hpp change.
        inline const int *glfwKeyTable(int &_count)
        {
            static constexpr int table[] = {
                // Letters
                GLFW_KEY_A, GLFW_KEY_B, GLFW_KEY_C, GLFW_KEY_D, GLFW_KEY_E, GLFW_KEY_F,
                GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_I, GLFW_KEY_J, GLFW_KEY_K, GLFW_KEY_L,
                GLFW_KEY_M, GLFW_KEY_N, GLFW_KEY_O, GLFW_KEY_P, GLFW_KEY_Q, GLFW_KEY_R,
                GLFW_KEY_S, GLFW_KEY_T, GLFW_KEY_U, GLFW_KEY_V, GLFW_KEY_W, GLFW_KEY_X,
                GLFW_KEY_Y, GLFW_KEY_Z,
                // Numbers
                GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4,
                GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,
                // Function keys
                GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3, GLFW_KEY_F4, GLFW_KEY_F5, GLFW_KEY_F6,
                GLFW_KEY_F7, GLFW_KEY_F8, GLFW_KEY_F9, GLFW_KEY_F10, GLFW_KEY_F11, GLFW_KEY_F12,
                // Navigation
                GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT,
                GLFW_KEY_HOME, GLFW_KEY_END, GLFW_KEY_PAGE_UP, GLFW_KEY_PAGE_DOWN,
                GLFW_KEY_INSERT, GLFW_KEY_DELETE,
                // Modifiers
                GLFW_KEY_LEFT_SHIFT, GLFW_KEY_RIGHT_SHIFT,
                GLFW_KEY_LEFT_CONTROL, GLFW_KEY_RIGHT_CONTROL,
                GLFW_KEY_LEFT_ALT, GLFW_KEY_RIGHT_ALT,
                GLFW_KEY_LEFT_SUPER, GLFW_KEY_RIGHT_SUPER,
                // Special
                GLFW_KEY_SPACE, GLFW_KEY_ENTER, GLFW_KEY_BACKSPACE,
                GLFW_KEY_TAB, GLFW_KEY_ESCAPE, GLFW_KEY_CAPS_LOCK,
                // Punctuation
                GLFW_KEY_MINUS, GLFW_KEY_EQUAL,
                GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET,
                GLFW_KEY_BACKSLASH, GLFW_KEY_SEMICOLON,
                GLFW_KEY_APOSTROPHE, GLFW_KEY_GRAVE_ACCENT,
                GLFW_KEY_COMMA, GLFW_KEY_PERIOD, GLFW_KEY_SLASH,
                // System
                GLFW_KEY_PRINT_SCREEN, GLFW_KEY_SCROLL_LOCK, GLFW_KEY_PAUSE,
                // Numpad
                GLFW_KEY_KP_0, GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3, GLFW_KEY_KP_4,
                GLFW_KEY_KP_5, GLFW_KEY_KP_6, GLFW_KEY_KP_7, GLFW_KEY_KP_8, GLFW_KEY_KP_9,
                GLFW_KEY_KP_ADD, GLFW_KEY_KP_SUBTRACT, GLFW_KEY_KP_MULTIPLY, GLFW_KEY_KP_DIVIDE,
                GLFW_KEY_KP_ENTER, GLFW_KEY_KP_DECIMAL, GLFW_KEY_NUM_LOCK,
            };
            // Si Keys.hpp gagne/perd une entree sans que cette table suive, ca
            // casse ici a la compilation plutot qu'en silence a l'execution.
            static_assert(sizeof(table) / sizeof(table[0]) ==
                              static_cast<int>(ee::input::Key::NumLock) + 1,
                          "glfwKeyTable() desynchronise de l'enum ee::input::Key");
            _count = static_cast<int>(sizeof(table) / sizeof(table[0]));
            return table;
        }

        // Lit GLFW chaque frame et nourrit InputManager via les Sync* (pas de
        // fenetre SDL3 ici -> pas d'update()/event-pump SDL possible).
        inline void pollInput(GLFWwindow *_win)
        {
            using ee::input::InputManager;
            InputManager &im = InputManager::getInstance();
            int keyCount = 0;
            const int *table = glfwKeyTable(keyCount);

            static std::unordered_map<int, bool> s_prevKey;
            for (int i = 0; i < keyCount; ++i)
            {
                int glfwKey = table[i];
                bool now = glfwGetKey(_win, glfwKey) == GLFW_PRESS;
                bool was = s_prevKey[glfwKey];
                im.SyncKey(static_cast<ee::input::Key>(i), now && !was, now && was, !now && was);
                s_prevKey[glfwKey] = now;
            }

            static const int s_mouseTable[] = {
                GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_MIDDLE, GLFW_MOUSE_BUTTON_RIGHT,
                GLFW_MOUSE_BUTTON_4, GLFW_MOUSE_BUTTON_5, // X1, X2
            };
            static std::unordered_map<int, bool> s_prevMouse;
            for (int i = 0; i < 5; ++i)
            {
                int glfwBtn = s_mouseTable[i];
                bool now = glfwGetMouseButton(_win, glfwBtn) == GLFW_PRESS;
                bool was = s_prevMouse[glfwBtn];
                im.SyncMouseButton(static_cast<ee::input::MouseButton>(i), now && !was, now && was, !now && was);
                s_prevMouse[glfwBtn] = now;
            }

            double mx, my;
            glfwGetCursorPos(_win, &mx, &my);
            im.SyncMousePosition(static_cast<float>(mx), static_cast<float>(my));

            // Manette (1ere connectee) : API gamepad de GLFW, independante de SDL.
            GLFWgamepadstate pad;
            if (glfwGetGamepadState(GLFW_JOYSTICK_1, &pad))
            {
                static const int s_padButtonMap[] = {
                    GLFW_GAMEPAD_BUTTON_A, GLFW_GAMEPAD_BUTTON_B,
                    GLFW_GAMEPAD_BUTTON_X, GLFW_GAMEPAD_BUTTON_Y,
                    GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
                    GLFW_GAMEPAD_BUTTON_LEFT_THUMB, GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
                    GLFW_GAMEPAD_BUTTON_START, GLFW_GAMEPAD_BUTTON_BACK,
                    GLFW_GAMEPAD_BUTTON_GUIDE,
                    GLFW_GAMEPAD_BUTTON_DPAD_UP, GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
                    GLFW_GAMEPAD_BUTTON_DPAD_LEFT, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
                };
                static std::unordered_map<int, bool> s_prevPad;
                for (int i = 0; i < 15; ++i)
                {
                    int glfwBtn = s_padButtonMap[i];
                    bool now = pad.buttons[glfwBtn] == GLFW_PRESS;
                    bool was = s_prevPad[glfwBtn];
                    im.SyncGamepadButton(static_cast<ee::input::GamepadButton>(i), now && !was, now && was, !now && was);
                    s_prevPad[glfwBtn] = now;
                }

                static const int s_padAxisMap[] = {
                    GLFW_GAMEPAD_AXIS_LEFT_X, GLFW_GAMEPAD_AXIS_LEFT_Y,
                    GLFW_GAMEPAD_AXIS_RIGHT_X, GLFW_GAMEPAD_AXIS_RIGHT_Y,
                    GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER,
                };
                for (int i = 0; i < 6; ++i)
                    im.SyncGamepadAxis(static_cast<ee::input::GamepadAxis>(i), pad.axes[s_padAxisMap[i]]);
            }
        }
    }

    inline int run(const Config &_cfg,
                   const RegisterComponentsFn &_registerComponents,
                   const RegisterSystemsFn &_registerSystems)
    {
        Config cfg = _cfg;
        if (cfg.projectRoot.empty())
            cfg.projectRoot = detail::defaultProjectRoot();

        // 1) Scene authoree.
        std::string scenePath = cfg.projectRoot + "/Assets/ScenesDatas/" + cfg.sceneName + ".json";
        std::optional<SceneInfo> scene = loadScene(scenePath);
        if (!scene)
        {
            std::fprintf(stderr, "[runtime] scene introuvable : %s\n", scenePath.c_str());
            return 1;
        }
        ee::core::setMeshBaseDir(cfg.projectRoot);

        // 2) Composants connus (moteur + jeu).
        ee::scene::WorldRegistry reg;
        ee::scene::registerEngineComponents(reg);
        if (_registerComponents)
            _registerComponents(reg);

        ee::ecs::World world;

        // 3) Systemes AVANT le chargement (le flush les peuple).
        std::vector<SystemInfo> schedule =
            loadSystemsInDir(cfg.projectRoot + "/systems");
        std::vector<SystemInfo> engineSchedule =
            ee::scene::engineSystemInfos();
        schedule.insert(schedule.end(), engineSchedule.begin(), engineSchedule.end());
        std::stable_sort(schedule.begin(), schedule.end(), [](const SystemInfo& _a, const SystemInfo& _b)
        {return _a.priority < _b.priority;});
        
        ee::scene::SystemHost host;

        ee::scene::registerEngineSystems(host);
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
        GLFWwindow *win = glfwCreateWindow(cfg.width, cfg.height,
                                           cfg.windowTitle.c_str(), nullptr, nullptr);
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
