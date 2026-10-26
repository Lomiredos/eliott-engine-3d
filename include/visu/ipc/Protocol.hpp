#pragma once

// ---------------------------------------------------------------------------
// Protocole du lien editeur <-> jeu (messages JSON sur ee::ipc::Socket).
//
// Chaque message a un champ "t" (son type) :
//   editeur -> jeu : resize, input, set, stop
//   jeu -> editeur : ready, snap
// Les images ne passent PAS par ici : voir SharedFrame.
// ---------------------------------------------------------------------------

#include "visu/core/SceneInfo.hpp"
#include "visu/core/SceneJson.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace ee::ipc
{
    // Variable d'environnement posee par l'editeur au lancement du jeu.
    inline constexpr const char *kEditorHostEnv = "EE_EDITOR_HOST";

    enum class PlayMode
    {
        Preview, // fenetre cachee, image dans la preview de l'editeur
        Window   // fenetre de jeu visible, inputs lus par le jeu
    };

    // Contenu de EE_EDITOR_HOST : "<port>;<preview|window>;<nom shm>".
    struct HostInfo
    {
        uint16_t port = 0;
        PlayMode mode = PlayMode::Preview;
        std::string shmName; // vide en mode Window
    };

    inline std::string formatHostInfo(const HostInfo &_h)
    {
        return std::to_string(_h.port) + ";" + (_h.mode == PlayMode::Window ? "window" : "preview") +
               ";" + _h.shmName;
    }

    inline std::optional<HostInfo> parseHostInfo(const char *_env)
    {
        if (!_env || !*_env)
            return std::nullopt;
        std::stringstream ss(_env);
        std::string port, mode, shm;
        std::getline(ss, port, ';');
        std::getline(ss, mode, ';');
        std::getline(ss, shm, ';');
        HostInfo h;
        try
        {
            h.port = static_cast<uint16_t>(std::stoi(port));
        }
        catch (const std::exception &)
        {
            return std::nullopt;
        }
        h.mode = mode == "window" ? PlayMode::Window : PlayMode::Preview;
        h.shmName = shm;
        return h;
    }

    // --- Etat d'input envoye par l'editeur (mode Preview) ---
    struct InputState
    {
        std::vector<int> keysDown; // indices de ee::input::Key enfoncees
        float mouseX = 0.0f, mouseY = 0.0f; // pixels, relatifs a la preview
        bool mouse[5] = {false, false, false, false, false};
    };

    // --- editeur -> jeu ---
    inline nlohmann::json msgResize(int _w, int _h) { return {{"t", "resize"}, {"w", _w}, {"h", _h}}; }

    inline nlohmann::json msgInput(const InputState &_in)
    {
        nlohmann::json j = {{"t", "input"}, {"keys", _in.keysDown}, {"mx", _in.mouseX}, {"my", _in.mouseY}};
        j["mb"] = nlohmann::json::array();
        for (bool b : _in.mouse)
            j["mb"].push_back(b);
        return j;
    }

    inline InputState inputFromMsg(const nlohmann::json &_j)
    {
        InputState in;
        in.keysDown = _j.value("keys", std::vector<int>{});
        in.mouseX = _j.value("mx", 0.0f);
        in.mouseY = _j.value("my", 0.0f);
        if (_j.contains("mb") && _j["mb"].is_array())
            for (size_t i = 0; i < 5 && i < _j["mb"].size(); ++i)
                in.mouse[i] = _j["mb"][i].get<bool>();
        return in;
    }

    // Modifie un champ d'un composant d'une entite (index = celui de la scene).
    inline nlohmann::json msgSetField(int _entity, const std::string &_comp,
                                      const std::string &_field, const FieldValue &_value)
    {
        return {{"t", "set"}, {"e", _entity}, {"c", _comp}, {"f", _field}, {"v", fieldValueToJson(_value)}};
    }

    inline nlohmann::json msgStop() { return {{"t", "stop"}}; }

    // --- jeu -> editeur ---
    inline nlohmann::json msgReady() { return {{"t", "ready"}}; }

    inline nlohmann::json msgSnapshot(const SceneInfo &_scene)
    {
        return {{"t", "snap"}, {"scene", sceneToJson(_scene)}};
    }

    inline std::string msgType(const nlohmann::json &_j) { return _j.value("t", ""); }
}
