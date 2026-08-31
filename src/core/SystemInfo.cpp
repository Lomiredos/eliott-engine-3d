#include "visu/core/SystemInfo.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

std::optional<SystemInfo> loadSystemInfo(const std::filesystem::path &jsonPath)
{
    std::ifstream f(jsonPath);
    if (!f)
        return std::nullopt;

    try
    {
        nlohmann::json j = nlohmann::json::parse(f);

        SystemInfo info;
        info.name = j.value("name", "");
        info.priority = j.value("priority", 0);
        info.category = j.value("category", "");
        if (j.contains("components"))
            info.components = j["components"].get<std::vector<std::string>>();

        return info;
    }
    catch (const std::exception &)
    {
        return std::nullopt; // JSON malforme
    }
}

std::vector<SystemInfo> loadSystemsInDir(const std::filesystem::path &dir)
{
    std::vector<SystemInfo> systems;

    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec))
        return systems; // dossier absent -> aucun systeme

    for (const auto &entry : std::filesystem::directory_iterator(dir, ec))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
            continue;
        if (auto info = loadSystemInfo(entry.path()))
            systems.push_back(*info);
    }

    // Ordre d'execution = priority croissante (stable pour l'egalite).
    std::stable_sort(systems.begin(), systems.end(),
                     [](const SystemInfo &a, const SystemInfo &b)
                     { return a.priority < b.priority; });

    return systems;
}
