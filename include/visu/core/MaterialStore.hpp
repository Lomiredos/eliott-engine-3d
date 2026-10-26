#pragma once

#include "visu/core/Color3.hpp"

#include <filesystem>
#include <optional>
#include <string>

// ---------------------------------------------------------------------------
// Materiaux (.mat) : fichiers JSON partages, ex. Assets/Materials/Brique.mat,
// references par le MaterialComponent des entites. PBR metal/rugosite.
//
//   {
//     "albedo": "#ffffff", "metallic": 0.0, "roughness": 0.6, "ao": 1.0,
//     "emissive": "#000000", "emissiveIntensity": 1.0, "normalStrength": 1.0,
//     "tiling": [1, 1], "offset": [0, 0],
//     "maps": { "albedo": "Assets/Textures/brique_albedo.png", "normal": "...",
//               "metallic": "", "roughness": "...", "ao": "...", "emissive": "" }
//   }
//
// Chaque valeur multiplie sa map (ex. roughness 1 + map = la map telle quelle).
// Chemins relatifs a la racine du projet (voir assetPath).
// ---------------------------------------------------------------------------
namespace ee::core
{
    // Ordre = ee::render::MapSlot.
    inline constexpr int kMaterialMapCount = 6;
    inline constexpr const char *kMaterialMapNames[kMaterialMapCount] = {
        "albedo", "normal", "metallic", "roughness", "ao", "emissive"};

    struct MaterialAsset
    {
        ee::Color3 albedo{1.0f, 1.0f, 1.0f};
        float metallic = 0.0f;
        float roughness = 0.6f;
        float ao = 1.0f;
        ee::Color3 emissive{0.0f, 0.0f, 0.0f};
        float emissiveIntensity = 1.0f;
        float normalStrength = 1.0f;
        float tiling[2] = {1.0f, 1.0f};
        float offset[2] = {0.0f, 0.0f};
        std::string maps[kMaterialMapCount]; // "" = pas de map
    };

    std::string materialToJsonText(const MaterialAsset &_m);
    std::optional<MaterialAsset> materialFromJsonText(const std::string &_text);

    // Lecture/ecriture d'un fichier .mat (chemin complet).
    std::optional<MaterialAsset> loadMaterialFile(const std::filesystem::path &_file);
    bool saveMaterialFile(const std::filesystem::path &_file, const MaterialAsset &_m);

    // Materiau d'un chemin d'asset (relatif au projet), mis en cache et
    // relu si le fichier change (verifie au plus 1 fois/s). nullptr si le
    // fichier n'existe pas ou est invalide.
    const MaterialAsset *getMaterial(const std::string &_path);

    // Editeur : remplace tout de suite la version en cache (apercu en direct
    // pendant qu'on regle le materiau, avant l'ecriture du fichier).
    void setMaterialInCache(const std::string &_path, const MaterialAsset &_m);
}
