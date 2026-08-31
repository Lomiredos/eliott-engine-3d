#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Modele d'un systeme tel qu'il est AUTHORE (sidecar JSON), partage par
// l'editeur (qui l'ecrit) et le jeu (qui le lit pour planifier l'execution).
//
// `components` = la SIGNATURE : une entite est traitee par le systeme si elle
// porte tous ces composants. `priority` ordonne l'execution.
// ---------------------------------------------------------------------------

struct SystemInfo
{
    std::string name;
    std::vector<std::string> components;
    int priority = 0;
    std::string category; // "gameplay" | "ui"
};

// Charge un sidecar <chemin>.json. nullopt si absent ou illisible.
std::optional<SystemInfo> loadSystemInfo(const std::filesystem::path &jsonPath);

// Charge TOUS les sidecars *.json d'un dossier, tries par priority croissante.
// Dossier absent -> liste vide (pas une erreur).
std::vector<SystemInfo> loadSystemsInDir(const std::filesystem::path &dir);
