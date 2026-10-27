#include "visu/core/MaterialStore.hpp"

#include "visu/core/MeshStore.hpp" // assetPath

#include <nlohmann/json.hpp>

#include <chrono>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace ee::core
{
    std::string materialToJsonText(const MaterialAsset &_m)
    {
        nlohmann::json j;
        j["albedo"] = ee::toHex(_m.albedo);
        j["metallic"] = _m.metallic;
        j["roughness"] = _m.roughness;
        j["ao"] = _m.ao;
        j["emissive"] = ee::toHex(_m.emissive);
        j["emissiveIntensity"] = _m.emissiveIntensity;
        j["normalStrength"] = _m.normalStrength;
        j["tiling"] = {_m.tiling[0], _m.tiling[1]};
        j["offset"] = {_m.offset[0], _m.offset[1]};
        j["transparent"] = _m.transparent;
        j["opacity"] = _m.opacity;
        for (int i = 0; i < kMaterialMapCount; ++i)
            j["maps"][kMaterialMapNames[i]] = _m.maps[i];
        return j.dump(2) + "\n";
    }

    std::optional<MaterialAsset> materialFromJsonText(const std::string &_text)
    {
        nlohmann::json j = nlohmann::json::parse(_text, nullptr, false);
        if (!j.is_object())
            return std::nullopt;

        MaterialAsset m;
        try
        {
            ee::fromHex(j.value("albedo", ee::toHex(m.albedo)), m.albedo);
            m.metallic = j.value("metallic", m.metallic);
            m.roughness = j.value("roughness", m.roughness);
            m.ao = j.value("ao", m.ao);
            ee::fromHex(j.value("emissive", ee::toHex(m.emissive)), m.emissive);
            m.emissiveIntensity = j.value("emissiveIntensity", m.emissiveIntensity);
            m.normalStrength = j.value("normalStrength", m.normalStrength);
            if (j.contains("tiling") && j["tiling"].is_array() && j["tiling"].size() == 2)
                for (int k = 0; k < 2; ++k)
                    m.tiling[k] = j["tiling"][k].get<float>();
            if (j.contains("offset") && j["offset"].is_array() && j["offset"].size() == 2)
                for (int k = 0; k < 2; ++k)
                    m.offset[k] = j["offset"][k].get<float>();
            m.transparent = j.value("transparent", m.transparent);
            m.opacity = j.value("opacity", m.opacity);
            if (j.contains("maps") && j["maps"].is_object())
                for (int i = 0; i < kMaterialMapCount; ++i)
                    m.maps[i] = j["maps"].value(kMaterialMapNames[i], std::string());
        }
        catch (const std::exception &)
        {
            return std::nullopt; // mauvais type de valeur
        }
        return m;
    }

    std::optional<MaterialAsset> loadMaterialFile(const std::filesystem::path &_file)
    {
        std::ifstream f(_file);
        if (!f)
            return std::nullopt;
        std::stringstream ss;
        ss << f.rdbuf();
        return materialFromJsonText(ss.str());
    }

    bool saveMaterialFile(const std::filesystem::path &_file, const MaterialAsset &_m)
    {
        std::error_code ec;
        if (_file.has_parent_path())
            std::filesystem::create_directories(_file.parent_path(), ec);
        std::ofstream f(_file, std::ios::binary);
        if (!f)
            return false;
        f << materialToJsonText(_m);
        return f.good();
    }

    namespace
    {
        struct Cached
        {
            std::optional<MaterialAsset> mat;
            std::filesystem::file_time_type mtime{};
            std::chrono::steady_clock::time_point lastCheck{};
            bool checked = false;
        };
        std::unordered_map<std::string, Cached> g_cache; // cle : chemin complet
    }

    const MaterialAsset *getMaterial(const std::string &_path)
    {
        if (_path.empty())
            return nullptr;
        const std::filesystem::path full = assetPath(_path);
        Cached &c = g_cache[full.string()];

        auto now = std::chrono::steady_clock::now();
        if (c.checked && now - c.lastCheck < std::chrono::seconds(1))
            return c.mat ? &*c.mat : nullptr;
        c.lastCheck = now;

        std::error_code ec;
        auto mtime = std::filesystem::last_write_time(full, ec);
        if (ec)
        {
            c.checked = true;
            c.mat.reset(); // fichier supprime
            return nullptr;
        }
        if (!c.checked || mtime != c.mtime)
        {
            c.mtime = mtime;
            c.mat = loadMaterialFile(full);
        }
        c.checked = true;
        return c.mat ? &*c.mat : nullptr;
    }

    void setMaterialInCache(const std::string &_path, const MaterialAsset &_m)
    {
        Cached &c = g_cache[assetPath(_path).string()];
        c.mat = _m;
        c.checked = true;
        c.lastCheck = std::chrono::steady_clock::now();
        std::error_code ec;
        c.mtime = std::filesystem::last_write_time(assetPath(_path), ec);
    }
}
