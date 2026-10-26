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

#include "visu/runtime/GameSession.hpp"
#include "visu/render/GLRenderer.hpp"

#include "input/InputManager.hpp"
#include "input/Keys.hpp"

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "visu/ipc/Protocol.hpp"
#include "visu/ipc/SharedFrame.hpp"
#include "visu/ipc/Socket.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace ee::runtime
{
    namespace detail
    {
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

        // Nourrit InputManager depuis un etat "enfonce ou pas" par touche et
        // par bouton : appui/maintien/relache sont deduits de la frame d'avant.
        // Meme chemin que l'etat vienne de GLFW (fenetre du jeu) ou de l'editeur.
        inline void applyKeysAndMouse(const std::vector<bool> &_keys, const bool _mouse[5],
                                      float _mx, float _my)
        {
            using ee::input::InputManager;
            InputManager &im = InputManager::getInstance();

            static std::vector<bool> s_prevKey;
            s_prevKey.resize(_keys.size(), false);
            for (size_t i = 0; i < _keys.size(); ++i)
            {
                bool now = _keys[i], was = s_prevKey[i];
                im.SyncKey(static_cast<ee::input::Key>(i), now && !was, now && was, !now && was);
                s_prevKey[i] = now;
            }

            static bool s_prevMouse[5] = {false, false, false, false, false};
            for (int i = 0; i < 5; ++i)
            {
                bool now = _mouse[i], was = s_prevMouse[i];
                im.SyncMouseButton(static_cast<ee::input::MouseButton>(i), now && !was, now && was, !now && was);
                s_prevMouse[i] = now;
            }

            im.SyncMousePosition(_mx, _my);
        }

        // Manette (1ere connectee) : API gamepad de GLFW, independante de SDL.
        // Lue par le jeu lui-meme dans les deux modes.
        inline void pollGamepad()
        {
            using ee::input::InputManager;
            InputManager &im = InputManager::getInstance();

            GLFWgamepadstate pad;
            if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &pad))
                return;

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

        // Lit GLFW chaque frame et nourrit InputManager via les Sync* (pas de
        // fenetre SDL3 ici -> pas d'update()/event-pump SDL possible).
        // Touches lues par POSITION physique (GLFW_KEY_* = disposition US).
        inline void pollInput(GLFWwindow *_win)
        {
            int keyCount = 0;
            const int *table = glfwKeyTable(keyCount);
            std::vector<bool> keys(static_cast<size_t>(keyCount));
            for (int i = 0; i < keyCount; ++i)
                keys[i] = glfwGetKey(_win, table[i]) == GLFW_PRESS;

            static const int s_mouseTable[] = {
                GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_MIDDLE, GLFW_MOUSE_BUTTON_RIGHT,
                GLFW_MOUSE_BUTTON_4, GLFW_MOUSE_BUTTON_5, // X1, X2
            };
            bool mouse[5];
            for (int i = 0; i < 5; ++i)
                mouse[i] = glfwGetMouseButton(_win, s_mouseTable[i]) == GLFW_PRESS;

            double mx, my;
            glfwGetCursorPos(_win, &mx, &my);
            applyKeysAndMouse(keys, mouse, static_cast<float>(mx), static_cast<float>(my));
            pollGamepad();
        }

        // Meme chose depuis l'etat envoye par l'editeur (mode preview).
        inline void applyRemoteInput(const ee::ipc::InputState &_in)
        {
            int keyCount = 0;
            glfwKeyTable(keyCount);
            std::vector<bool> keys(static_cast<size_t>(keyCount), false);
            for (int k : _in.keysDown)
                if (k >= 0 && k < keyCount)
                    keys[k] = true;
            applyKeysAndMouse(keys, _in.mouse, _in.mouseX, _in.mouseY);
            pollGamepad();
        }

        // ---------------------------------------------------------------
        // Lien avec l'editeur (EE_EDITOR_HOST defini) : recoit les ordres
        // (stop, taille, input, champs modifies) et renvoie regulierement
        // l'etat du World (snapshot) pour l'inspecteur.
        // ---------------------------------------------------------------
        class EditorLink
        {
        public:
            bool connect(uint16_t _port)
            {
                m_sock = ee::ipc::Socket::connect(_port);
                if (!m_sock.valid())
                    return false;
                m_sock.send(ee::ipc::msgReady());
                return true;
            }

            // false si l'editeur demande l'arret ou a coupe la connexion.
            bool update(GameSession &_game, double _now)
            {
                std::vector<nlohmann::json> msgs;
                bool alive = m_sock.poll(msgs);
                for (const nlohmann::json &m : msgs)
                {
                    std::string t = ee::ipc::msgType(m);
                    if (t == "stop")
                        return false;
                    if (t == "resize")
                    {
                        m_width = m.value("w", m_width);
                        m_height = m.value("h", m_height);
                    }
                    else if (t == "input")
                        m_input = ee::ipc::inputFromMsg(m);
                    else if (t == "set")
                        applySet(_game, m);
                }

                if (_now - m_lastSnapshot >= 0.05) // ~20 Hz suffit a l'inspecteur
                {
                    m_lastSnapshot = _now;
                    m_sock.send(ee::ipc::msgSnapshot(ee::scene::snapshotWorld(
                        _game.world(), _game.entities(), _game.scene(), _game.registry())));
                }
                return alive;
            }

            const ee::ipc::InputState &input() const { return m_input; }
            int width() const { return m_width; }
            int height() const { return m_height; }

        private:
            void applySet(GameSession &_game, const nlohmann::json &_m)
            {
                int e = _m.value("e", -1);
                if (e < 0 || e >= static_cast<int>(_game.entities().size()) || !_m.contains("v"))
                    return;
                std::optional<FieldValue> v = fieldValueFromJson(_m["v"]);
                const ee::scene::WorldRegistry::Entry *entry = _game.registry().find(_m.value("c", ""));
                if (!v || !entry)
                    return;
                ComponentInstance patch;
                patch.name = _m.value("c", "");
                patch.values[_m.value("f", "")] = *v;
                entry->patch(_game.world(), _game.entities()[e], patch);
            }

            ee::ipc::Socket m_sock;
            ee::ipc::InputState m_input;
            int m_width = 0, m_height = 0;
            double m_lastSnapshot = -1.0;
        };

        // Mode preview : fenetre cachee (juste pour le contexte GL), rendu
        // dans un FBO, image copiee dans la memoire partagee de l'editeur.
        inline int runEmbedded(GameSession &_game, const ee::ipc::HostInfo &_host)
        {
            EditorLink link;
            if (!link.connect(_host.port))
            {
                std::fprintf(stderr, "[runtime] connexion a l'editeur impossible (port %u)\n", _host.port);
                return 1;
            }
            ee::ipc::SharedFrame frame;
            if (!frame.open(_host.shmName))
            {
                std::fprintf(stderr, "[runtime] memoire partagee introuvable : %s\n", _host.shmName.c_str());
                return 1;
            }

            if (!glfwInit())
            {
                std::fprintf(stderr, "[runtime] glfwInit a echoue\n");
                return 1;
            }
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            GLFWwindow *win = glfwCreateWindow(64, 64, "preview", nullptr, nullptr);
            if (!win)
            {
                std::fprintf(stderr, "[runtime] creation du contexte GL a echoue\n");
                glfwTerminate();
                return 1;
            }
            glfwMakeContextCurrent(win);
            glfwSwapInterval(0);

            ee::render::GLRenderer renderer; // offscreen (FBO) par defaut
            ee::render::Camera cam = defaultCamera();

            const double frameTime = 1.0 / 60.0; // pas de vsync sans fenetre visible
            double last = glfwGetTime();
            while (true)
            {
                double start = glfwGetTime();
                glfwPollEvents();
                if (!link.update(_game, start))
                    break;

                float dt = static_cast<float>(start - last);
                last = start;
                applyRemoteInput(link.input());
                _game.tick(dt);

                int w = std::min(link.width() > 0 ? link.width() : _game.config().width, ee::ipc::SharedFrame::kMaxWidth);
                int h = std::min(link.height() > 0 ? link.height() : _game.config().height, ee::ipc::SharedFrame::kMaxHeight);
                _game.render(renderer, cam, w, h);
                if (renderer.texture() != 0)
                {
                    glBindTexture(GL_TEXTURE_2D, renderer.texture());
                    glPixelStorei(GL_PACK_ALIGNMENT, 1);
                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.backBuffer());
                    glBindTexture(GL_TEXTURE_2D, 0);
                    frame.publish(w, h);
                }

                double spent = glfwGetTime() - start;
                if (spent < frameTime)
                    std::this_thread::sleep_for(std::chrono::duration<double>(frameTime - spent));
            }

            glfwDestroyWindow(win);
            glfwTerminate();
            return 0;
        }
    }

    inline int run(const Config &_cfg,
                   const RegisterComponentsFn &_registerComponents,
                   const RegisterSystemsFn &_registerSystems)
    {
        GameSession game;
        if (!game.load(_cfg, _registerComponents, _registerSystems))
            return 1;
        const Config &cfg = game.config();

        // Lance par l'editeur ? (sinon : jeu autonome, comme toujours)
        std::optional<ee::ipc::HostInfo> host = ee::ipc::parseHostInfo(std::getenv(ee::ipc::kEditorHostEnv));
        if (host && host->mode == ee::ipc::PlayMode::Preview)
            return detail::runEmbedded(game, *host);

        // Mode fenetre separee lancee par l'editeur : meme boucle, plus le lien.
        std::optional<detail::EditorLink> link;
        if (host)
        {
            link.emplace();
            if (!link->connect(host->port))
            {
                std::fprintf(stderr, "[runtime] connexion a l'editeur impossible (port %u)\n", host->port);
                return 1;
            }
        }

        // Fenetre + contexte OpenGL 3.3 core.
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
        ee::render::Camera cam = defaultCamera();

        // Boucle.
        double last = glfwGetTime();
        while (!glfwWindowShouldClose(win))
        {
            glfwPollEvents();
            if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(win, 1);

            double now = glfwGetTime();
            float dt = static_cast<float>(now - last);
            last = now;

            if (link && !link->update(game, now))
                break; // Stop demande par l'editeur (ou editeur ferme)

            detail::pollInput(win);
            game.tick(dt);

            int w, h;
            glfwGetFramebufferSize(win, &w, &h);
            game.render(renderer, cam, w, h);

            glfwSwapBuffers(win);
        }

        glfwDestroyWindow(win);
        glfwTerminate();
        return 0;
    }
}
