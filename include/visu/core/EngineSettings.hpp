#pragma once

#include "visu/render/Renderer.hpp"

#include <filesystem>
#include <string>

// ---------------------------------------------------------------------------
// Reglages du moteur d'un projet : Assets/EngineSettings.json, modifies dans
// l'onglet Reglages de l'editeur, lus par l'editeur et par le jeu.
//
//   { "render": { "maxPointLights": 8, "sunShadows": true, ... } }
//
// Valeur absente = valeur par defaut (RenderSettings) : un projet sans
// fichier tourne avec les reglages par defaut.
// ---------------------------------------------------------------------------
namespace ee::core
{
    inline constexpr const char *kEngineSettingsPath = "Assets/EngineSettings.json";

    struct EngineSettings
    {
        ee::render::RenderSettings render;
    };

    std::string engineSettingsToJsonText(const EngineSettings &_s);
    EngineSettings engineSettingsFromJsonText(const std::string &_text);

    // Lecture/ecriture (chemin complet).
    EngineSettings loadEngineSettingsFile(const std::filesystem::path &_file);
    bool saveEngineSettingsFile(const std::filesystem::path &_file, const EngineSettings &_s);

    // Reglages du projet courant (assetPath(kEngineSettingsPath)), mis en
    // cache et relus si le fichier change (verifie au plus 1 fois/s) : un
    // reglage modifie dans l'editeur s'applique au jeu en cours.
    const EngineSettings &engineSettings();

    // Editeur : remplace tout de suite la version en cache (apercu en direct
    // avant l'ecriture du fichier).
    void setEngineSettingsInCache(const EngineSettings &_s);
}
