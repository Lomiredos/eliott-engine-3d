#pragma once

// ---------------------------------------------------------------------------
// Une partie en cours, independante de la facon de l'afficher.
//
// Charge la scene dans un World typé, instancie les systemes (moteur + jeu),
// puis avance d'un pas (tick) et se dessine sur n'importe quel IRenderer.
// Ne connait ni fenetre ni input : c'est l'hote (run() fenetre, ou le lien
// avec l'editeur) qui choisit ou l'image va et d'ou viennent les touches.
// ---------------------------------------------------------------------------

#include "ecs/World.hpp"

#include "visu/core/SceneInfo.hpp"
#include "visu/core/MeshStore.hpp"
#include "visu/render/Renderer.hpp"
#include "visu/render/SplitScreen.hpp"
#include "visu/scene/WorldLoader.hpp"
#include "visu/scene/SystemHost.hpp"
#include "visu/scene/WorldRender.hpp"
#include "visu/scene/EngineSystems.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ee::runtime
{
    struct Config
    {
        // Assets/ScenesDatas/, Systems/, base des .obj. Laisse vide (defaut) :
        // deduit automatiquement de l'emplacement de l'executable (convention
        // <racine>/build/<exe>, celle qu'ecrit eliott-hub) -> marche quel que
        // soit le dossier courant au lancement (terminal, double-clic, Play
        // depuis l'editeur qui se place dans le dossier de l'exe...).
        std::string projectRoot;
        std::string sceneName = "BaseScene";  // Assets/ScenesDatas/<sceneName>.json
        std::string windowTitle = "Game";
        int width = 1000;
        int height = 700;
    };

    // Callbacks fournis par le jeu (générés par l'editeur).
    using RegisterComponentsFn = std::function<void(ee::scene::WorldRegistry &)>;
    using RegisterSystemsFn = std::function<void(ee::scene::SystemHost &)>;

    namespace detail
    {
        // Racine du projet deduite du chemin de l'executable (convention
        // <racine>/build/<exe>), independante du dossier courant.
        inline std::string defaultProjectRoot()
        {
#ifdef _WIN32
            char buffer[MAX_PATH];
            GetModuleFileNameA(nullptr, buffer, MAX_PATH);
            std::filesystem::path exe(buffer);
#else
            std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe");
#endif
            return exe.parent_path().parent_path().string(); // .../build/exe -> ...
        }
    }

    // Camera quand la scene n'a pas d'entite CameraComponent.
    inline ee::render::Camera defaultCamera()
    {
        ee::render::Camera cam;
        cam.pos = {0.0f, 3.0f, 12.0f};
        cam.yaw = -1.5707963f;
        cam.pitch = -0.15f;
        return cam;
    }

    class GameSession
    {
    public:
        // Charge scene + composants + systemes. false (et log) si echec.
        bool load(const Config &_cfg,
                  const RegisterComponentsFn &_registerComponents,
                  const RegisterSystemsFn &_registerSystems)
        {
            m_cfg = _cfg;
            if (m_cfg.projectRoot.empty())
                m_cfg.projectRoot = detail::defaultProjectRoot();

            // 1) Scene authoree.
            std::string scenePath = m_cfg.projectRoot + "/Assets/ScenesDatas/" + m_cfg.sceneName + ".json";
            std::optional<SceneInfo> scene = loadScene(scenePath);
            if (!scene)
            {
                std::fprintf(stderr, "[runtime] scene introuvable : %s\n", scenePath.c_str());
                return false;
            }
            m_scene = *scene;
            ee::core::setMeshBaseDir(m_cfg.projectRoot);

            // 2) Composants connus (moteur + jeu).
            ee::scene::registerEngineComponents(m_reg);
            if (_registerComponents)
                _registerComponents(m_reg);

            // 3) Systemes AVANT le chargement (le flush les peuple).
            std::vector<SystemInfo> schedule = loadSystemsInDir(m_cfg.projectRoot + "/Systems");
            std::vector<SystemInfo> engineSchedule = ee::scene::engineSystemInfos();
            schedule.insert(schedule.end(), engineSchedule.begin(), engineSchedule.end());
            std::stable_sort(schedule.begin(), schedule.end(), [](const SystemInfo &_a, const SystemInfo &_b)
                             { return _a.priority < _b.priority; });

            ee::scene::registerEngineSystems(m_host);
            if (_registerSystems)
                _registerSystems(m_host);
            m_host.build(m_world, m_reg, schedule);

            // 4) Scene -> World typé.
            m_entities = ee::scene::loadSceneIntoWorld(m_world, m_scene, m_reg);
            std::printf("[runtime] %zu entites, %zu systeme(s)\n", m_entities.size(), schedule.size());
            std::fflush(stdout);
            return true;
        }

        // Un pas de simulation : systemes puis commit des spawns/morts.
        void tick(float _dt)
        {
            m_host.updateAll(m_world, _dt);
            m_world.flush();
        }

        // Une vue par camera (ecran partage, voir splitScreen) ; sans
        // camera dans la scene, defaultCamera() en plein ecran.
        // _selected : index d'entite dont le collider est dessine (-1 = aucun).
        void render(ee::render::IRenderer &_r, int _w, int _h, int _selected = -1)
        {
            std::vector<ee::render::Camera> cams = cameras();
            std::vector<ee::render::Viewport> views =
                ee::render::splitScreen(static_cast<int>(cams.size()), _w, _h);

            _r.beginFrame(_w, _h);
            for (size_t i = 0; i < cams.size(); ++i)
            {
                _r.beginView(cams[i], views[i]);
                ee::scene::renderWorld(_r, m_world, m_entities, _selected);
            }
            _r.endScene();
        }

        // Cameras de la frame : entites CameraComponent (suivent leur
        // transform, donc deplacables par un systeme), sinon defaultCamera().
        std::vector<ee::render::Camera> cameras()
        {
            std::vector<ee::render::Camera> cams = ee::scene::worldCameras(m_world, m_entities);
            if (cams.empty())
                cams.push_back(defaultCamera());
            return cams;
        }

        const Config &config() const { return m_cfg; }
        const SceneInfo &scene() const { return m_scene; }
        ee::ecs::World &world() { return m_world; }
        const std::vector<ee::ecs::EntityID> &entities() const { return m_entities; }
        const ee::scene::WorldRegistry &registry() const { return m_reg; }

    private:
        Config m_cfg;
        SceneInfo m_scene;
        ee::scene::WorldRegistry m_reg;
        ee::ecs::World m_world;
        ee::scene::SystemHost m_host;
        std::vector<ee::ecs::EntityID> m_entities;
    };
}
