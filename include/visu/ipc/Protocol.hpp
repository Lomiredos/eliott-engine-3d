#pragma once

// ---------------------------------------------------------------------------
// Protocole du lien editeur <-> jeu (messages JSON sur ee::ipc::Socket).
//
// Chaque message a un champ "t" (son type) :
//   editeur -> jeu : resize, input, set, pause, select, stop
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

    // Pause : le jeu ne simule plus et n'ecoute plus les inputs, mais continue
    // de s'afficher et d'accepter les modifications de champs.
    inline nlohmann::json msgPause(bool _paused) { return {{"t", "pause"}, {"on", _paused}}; }

    // Entite selectionnee dans l'editeur (son collider est dessine), -1 = aucune.
    inline nlohmann::json msgSelect(int _entity) { return {{"t", "select"}, {"e", _entity}}; }

    inline nlohmann::json msgStop() { return {{"t", "stop"}}; }

    // Modifie un parametre de la machine a etats d'une entite (index = celui de la scene).
    // Trigger : true = setTrigger, false = resetTrigger.
    inline nlohmann::json msgFsmSet(int _entity, const std::string &_param, const nlohmann::json &_value)
    {
        return {{"t", "fsmset"}, {"e", _entity}, {"p", _param}, {"v", _value}};
    }

    // --- jeu -> editeur ---
    inline nlohmann::json msgReady() { return {{"t", "ready"}}; }

    struct FsmLiveState
    {
        int entity = -1;
        std::string path;
        std::string state;
        nlohmann::json params = nlohmann::json::object();
    };

    inline nlohmann::json msgSnapshot(const SceneInfo &_scene, const std::vector<FsmLiveState> &_fsm = {})
    {
        nlohmann::json j = {{"t", "snap"}, {"scene", sceneToJson(_scene)}};
        if (!_fsm.empty())
        {
            j["fsm"] = nlohmann::json::array();
            for (const FsmLiveState &f : _fsm)
                j["fsm"].push_back({{"e", f.entity}, {"path", f.path}, {"state", f.state}, {"params", f.params}});
        }
        return j;
    }

    inline std::vector<FsmLiveState> fsmStatesFromMsg(const nlohmann::json &_j)
    {
        std::vector<FsmLiveState> out;
        if (!_j.contains("fsm") || !_j["fsm"].is_array())
            return out;
        for (const nlohmann::json &f : _j["fsm"])
            if (f.is_object())
                out.push_back({f.value("e", -1), f.value("path", std::string()), f.value("state", std::string()),
                               f.contains("params") && f["params"].is_object() ? f["params"] : nlohmann::json::object()});
        return out;
    }

    inline std::string msgType(const nlohmann::json &_j) { return _j.value("t", ""); }
}
