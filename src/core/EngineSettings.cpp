#include "visu/core/EngineSettings.hpp"

#include "visu/core/MeshStore.hpp" // assetPath

#include <nlohmann/json.hpp>

#include <chrono>
#include <fstream>
#include <sstream>

namespace ee::core
{
    std::string engineSettingsToJsonText(const EngineSettings &_s)
    {
        const ee::render::RenderSettings &r = _s.render;
        nlohmann::json j;
        j["render"]["maxPointLights"] = r.maxPointLights;
        j["render"]["sunShadows"] = r.sunShadows;
        j["render"]["sunShadowResolution"] = r.sunShadowResolution;
        j["render"]["sunShadowDistance"] = r.sunShadowDistance;
        j["render"]["pointShadows"] = r.pointShadows;
        j["render"]["maxShadowedPointLights"] = r.maxShadowedPointLights;
        j["render"]["pointShadowResolution"] = r.pointShadowResolution;
        return j.dump(2) + "\n";
    }

    EngineSettings engineSettingsFromJsonText(const std::string &_text)
    {
        EngineSettings s;
        nlohmann::json j = nlohmann::json::parse(_text, nullptr, false);
        if (!j.is_object() || !j.contains("render") || !j["render"].is_object())
            return s;
        const nlohmann::json &jr = j["render"];
        ee::render::RenderSettings &r = s.render;
        // Valeur une par une : un champ de mauvais type garde son defaut.
        auto read = [&jr](const char *_key, auto &_value)
        {
            try
            {
                _value = jr.value(_key, _value);
            }
            catch (const std::exception &)
            {
            }
        };
        read("maxPointLights", r.maxPointLights);
        read("sunShadows", r.sunShadows);
        read("sunShadowResolution", r.sunShadowResolution);
        read("sunShadowDistance", r.sunShadowDistance);
        read("pointShadows", r.pointShadows);
        read("maxShadowedPointLights", r.maxShadowedPointLights);
        read("pointShadowResolution", r.pointShadowResolution);
        return s;
    }

    EngineSettings loadEngineSettingsFile(const std::filesystem::path &_file)
    {
        std::ifstream f(_file);
        if (!f)
            return {};
        std::stringstream ss;
        ss << f.rdbuf();
        return engineSettingsFromJsonText(ss.str());
    }

    bool saveEngineSettingsFile(const std::filesystem::path &_file, const EngineSettings &_s)
    {
        std::error_code ec;
        if (_file.has_parent_path())
            std::filesystem::create_directories(_file.parent_path(), ec);
        std::ofstream f(_file, std::ios::binary);
        if (!f)
            return false;
        f << engineSettingsToJsonText(_s);
        return f.good();
    }

    namespace
    {
        struct Cached
        {
            EngineSettings settings;
            std::string file; // chemin complet (change avec le projet)
            std::filesystem::file_time_type mtime{};
            std::chrono::steady_clock::time_point lastCheck{};
            bool checked = false;
        };
        Cached g_cache;
    }

    const EngineSettings &engineSettings()
    {
        const std::string file = assetPath(kEngineSettingsPath).string();
        auto now = std::chrono::steady_clock::now();
        if (g_cache.checked && g_cache.file == file &&
            now - g_cache.lastCheck < std::chrono::seconds(1))
            return g_cache.settings;
        g_cache.lastCheck = now;

        std::error_code ec;
        auto mtime = std::filesystem::last_write_time(file, ec);
        if (ec)
            mtime = {}; // pas de fichier : reglages par defaut
        if (!g_cache.checked || g_cache.file != file || mtime != g_cache.mtime)
        {
            g_cache.settings = ec ? EngineSettings{} : loadEngineSettingsFile(file);
            g_cache.mtime = mtime;
            g_cache.file = file;
        }
        g_cache.checked = true;
        return g_cache.settings;
    }

    void setEngineSettingsInCache(const EngineSettings &_s)
    {
        const std::string file = assetPath(kEngineSettingsPath).string();
        g_cache.settings = _s;
        g_cache.file = file;
        g_cache.checked = true;
        g_cache.lastCheck = std::chrono::steady_clock::now();
        std::error_code ec;
        auto mtime = std::filesystem::last_write_time(file, ec);
        g_cache.mtime = ec ? std::filesystem::file_time_type{} : mtime;
    }
}
