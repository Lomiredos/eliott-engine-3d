#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>


struct SystemInfo
{
    std::string name;
    std::vector<std::string> components;
    int priority = 0;
    std::string category;
};

std::optional<SystemInfo> loadSystemInfo(const std::filesystem::path &jsonPath);

// Meme lecture, depuis le texte JSON (ex. sidecars moteur embarques au build).
std::optional<SystemInfo> parseSystemInfo(const std::string &json);

std::vector<SystemInfo> loadSystemsInDir(const std::filesystem::path &dir);
