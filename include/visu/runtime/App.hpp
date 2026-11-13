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

#include "visu/core/Window.hpp"
#include "visu/runtime/GameSession.hpp"
#include "visu/fsm/API.hpp"
#include "visu/fsm/EntityAPI.hpp"
#include "visu/fsm/StateMachine.hpp"
#include "visu/fsm/Utils.hpp"
#include "visu/runtime/GlfwKeys.hpp"
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
    namespace detail {
        inline constexpr float kMaxDt = 0.05f;

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
                    else if (t == "pause")
                        m_paused = m.value("on", false);
                    else if (t == "select")
                        m_selected = m.value("e", -1);
                    else if (t == "set")
                        applySet(_game, m);
                    else if (t == "fsmset")
                        applyFsmSet(_game, m);
                }

                if (_now - m_lastSnapshot >= 0.05) // ~20 Hz suffit a l'inspecteur
                {
                    m_lastSnapshot = _now;
                    m_sock.send(ee::ipc::msgSnapshot(
                        ee::scene::snapshotWorld(_game.world(), _game.entities(), _game.scene(), _game.registry()),
                        fsmStates(_game)));
                }
                return alive;
            }

            const ee::ipc::InputState &input() const { return m_input; }
            bool paused() const { return m_paused; }
            int selected() const { return m_selected; }
            int width() const { return m_width; }
            int height() const { return m_height; }

        private:
            static std::vector<ee::ipc::FsmLiveState> fsmStates(GameSession& _game) {
                std::vector<ee::ipc::FsmLiveState> out;
                ee::ecs::World& world = _game.world();
                const std::vector<ee::ecs::EntityID>& ents = _game.entities();
                for (size_t i = 0; i < ents.size(); ++i) {
                    if (!world.hasComponent<StateMachineComponent>(ents[i])) continue;
                    ee::ipc::FsmLiveState live;
                    live.entity = static_cast<int>(i);
                    live.state = ee::fsm::currentStateName(world, ents[i]);
                    const StateMachineComponent& comp = *world.getComponent<StateMachineComponent>(ents[i]);
                    live.path = comp.path;
                    if (comp.def && comp.inst.params.size() == comp.def->params.size())
                        for (size_t p = 0; p < comp.def->params.size(); ++p)
                            std::visit([&](auto v) { live.params[comp.def->params[p].name] = v; }, comp.inst.params[p]);
                    out.push_back(std::move(live));
                }
                return out;
            }

            static void applyFsmSet(GameSession& _game, const nlohmann::json& _m) {
                const int e = _m.value("e", -1);
                const std::string name = _m.value("p", std::string());
                if (e < 0 || e >= static_cast<int>(_game.entities().size()) || !_m.contains("v")) return;
                ee::ecs::World& world = _game.world();
                const ee::ecs::EntityID id = _game.entities()[e];
                if (!world.hasComponent<StateMachineComponent>(id)) return;
                StateMachineComponent& comp = *world.getComponent<StateMachineComponent>(id);
                if (!ee::fsm::syncStateMachine(comp)) return;
                const int index = ee::fsm::findParam(*comp.def, name);
                if (index < 0) return;
                const nlohmann::json& v = _m["v"];
                switch (comp.def->params[index].type) {
                    case ee::fsm::ParamType::Bool:
                        if (v.is_boolean()) ee::fsm::setBool(*comp.def, comp.inst, index, v.get<bool>());
                        break;
                    case ee::fsm::ParamType::Trigger:
                        if (v.is_boolean() && v.get<bool>())
                            ee::fsm::setTrigger(*comp.def, comp.inst, index);
                        else
                            ee::fsm::resetTrigger(*comp.def, comp.inst, index);
                        break;
                    case ee::fsm::ParamType::Int:
                        if (v.is_number()) ee::fsm::setInt(*comp.def, comp.inst, index, v.get<int>());
                        break;
                    case ee::fsm::ParamType::Float:
                        if (v.is_number()) ee::fsm::setFloat(*comp.def, comp.inst, index, v.get<float>());
                        break;
                }
            }

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
                entry->patch(_game.world(), _game.entities()[e], patch, _game.entities());
            }

            ee::ipc::Socket m_sock;
            ee::ipc::InputState m_input;
            int m_width = 0, m_height = 0;
            double m_lastSnapshot = -1.0;
            bool m_paused = false;
            int m_selected = -1;
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

            ee::core::Window &window = ee::core::Window::getInstance();
            window.runtimeInit(_game.config().windowTitle, true);

            const double frameTime = 1.0 / 60.0; // pas de vsync sans fenetre visible
            double last = glfwGetTime();
            while (true)
            {
                double start = glfwGetTime();
                glfwPollEvents();
                if (!link.update(_game, start))
                    break;

                float dt = std::min(static_cast<float>(start - last), detail::kMaxDt);
                last = start;

                // Taille du panneau preview, connue des systemes des ce tick.
                int w = std::min(link.width() > 0 ? link.width() : _game.config().width, ee::ipc::SharedFrame::kMaxWidth);
                int h = std::min(link.height() > 0 ? link.height() : _game.config().height, ee::ipc::SharedFrame::kMaxHeight);
                window.runtimeSetSize(w, h);
                // Pas de vraie fenetre : titre / curseur / close sans effet.
                window.runtimeTakeTitleChange();
                window.runtimeTakeCursorChange();

                if (link.paused())
                    applyRemoteInput(ee::ipc::InputState{}); // en pause : plus aucun input
                else
                {
                    applyRemoteInput(link.input());
                    _game.tick(dt);
                }

                _game.render(renderer, w, h, link.selected());
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
    } // namespace detail

    inline int run(const Config &_cfg,
                   const RegisterComponentsFn &_registerComponents,
                   const RegisterSystemsFn &_registerSystems)
    {
        GameSession game;
        if (!game.load(_cfg, _registerComponents, _registerSystems))
            return 1;
        const Config &cfg = game.config();

        // Lance par l'editeur pour jouer dans sa preview ? Sinon (jeu seul, ou
        // Play en fenetre separee) : la fenetre du jeu, sans lien.
        std::optional<ee::ipc::HostInfo> host = ee::ipc::parseHostInfo(std::getenv(ee::ipc::kEditorHostEnv));
        if (host && host->mode == ee::ipc::PlayMode::Preview)
            return detail::runEmbedded(game, *host);

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

        ee::core::Window &window = ee::core::Window::getInstance();
        window.runtimeInit(cfg.windowTitle, false);

        // Boucle.
        double last = glfwGetTime();
        while (!glfwWindowShouldClose(win))
        {
            glfwPollEvents();
            if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(win, 1);

            double now = glfwGetTime();
            float dt = std::min(static_cast<float>(now - last), detail::kMaxDt);
            last = now;

            detail::pollInput(win);

            int w, h;
            glfwGetFramebufferSize(win, &w, &h);
            window.runtimeSetSize(w, h);

            game.tick(dt);

            // Demandes faites par les systemes pendant le tick.
            if (window.runtimeTakeTitleChange())
                glfwSetWindowTitle(win, window.title().c_str());
            if (window.runtimeTakeCursorChange())
                glfwSetInputMode(win, GLFW_CURSOR,
                                 window.isCursorLocked() ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            if (window.runtimeCloseRequested())
                glfwSetWindowShouldClose(win, 1);

            game.render(renderer, w, h);

            glfwSwapBuffers(win);
        }

        glfwDestroyWindow(win);
        glfwTerminate();
        return 0;
    }
}
