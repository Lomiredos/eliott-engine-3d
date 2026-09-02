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

std::vector<SystemInfo> loadSystemsInDir(const std::filesystem::path &dir);
