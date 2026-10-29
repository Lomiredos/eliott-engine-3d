cmake_minimum_required(VERSION 3.20)
project(ee-core LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20) # eliott-ecs utilise des trucs C++20 (unordered_set::contains)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Autonome : quand ee-core est le point d'entree (projet etudiant via
# eliott-hub), personne d'autre ne fournit glad/glfw/nlohmann_json -> on les
# fournit nous-memes. Quand ee-core est embarque dans ee-visu (qui les fournit
# deja via FetchContent/une lib locale), on ne les redefinit pas.
# ---------------------------------------------------------------------------
if(NOT TARGET glad::glad)
    add_library(glad STATIC extern/glad/src/glad.c)
    target_include_directories(glad PUBLIC extern/glad/include)
    find_package(OpenGL REQUIRED)
    target_link_libraries(glad PUBLIC OpenGL::GL)
    add_library(glad::glad ALIAS glad)
endif()

if(NOT TARGET glfw)
    include(FetchContent)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

if(NOT TARGET nlohmann_json::nlohmann_json)
    include(FetchContent)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(json)
endif()

# ---------------------------------------------------------------------------
# Reflexion -> catalogue de composants (Components.json). A part d'ee-core :
# gen_components n'a besoin que de ca (+ headers), pas du rendu, de glfw...
# ---------------------------------------------------------------------------
add_library(ee-reflect STATIC src/reflect/CatalogGen.cpp)
target_include_directories(ee-reflect PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/eliott-math/include
)
target_link_libraries(ee-reflect PUBLIC nlohmann_json::nlohmann_json)

# ---------------------------------------------------------------------------
# Trois bibliotheques, de la plus legere a la plus complete :
#   ee-format : le contrat editeur <-> jeu et les donnees (scene, JSON,
#               hierarchie, materiaux, maillages, reglages, lien IPC). Ni GL,
#               ni ECS, ni input.
#   ee-render : rendu OpenGL et picking d'une scene (SceneInfo). Pas d'ECS.
#   ee-core   : tout le runtime du jeu (ECS, input/SDL, systemes, boucle).
# L'editeur (ee-visu) s'arrete a ee-render (EE_BUILD_RUNTIME=OFF) ; les
# projets lient ee-core, qui amene les deux autres.
# ---------------------------------------------------------------------------
option(EE_BUILD_RUNTIME "Construit ee-core (ECS, input, SDL) en plus d'ee-format/ee-render" ON)

add_library(ee-format STATIC
    src/core/SceneInfo.cpp
    src/core/SceneQuery.cpp
    src/core/SystemInfo.cpp
    src/core/EngineSettings.cpp
    src/core/MeshStore.cpp
    src/core/MaterialStore.cpp
    src/ipc/Socket.cpp
    src/ipc/SharedFrame.cpp
)
target_include_directories(ee-format PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/eliott-math/include
    # input/Keys.hpp seul (enum, sans dependance) : table des touches partagee
    # par l'editeur et le jeu (visu/runtime/GlfwKeys.hpp), sans lier eliott-input.
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/eliott-input/include
)
target_link_libraries(ee-format PUBLIC nlohmann_json::nlohmann_json)

# Lien editeur <-> jeu (visu/ipc) : sockets Winsock sous Windows, shm_open
# (librt) sous Linux.
if(WIN32)
    target_link_libraries(ee-format PUBLIC ws2_32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(ee-format PUBLIC rt)
endif()

add_library(ee-render STATIC
    src/render/GLRenderer.cpp
    src/render/RendererService.cpp
    src/render/StbImage.cpp
    src/systems/RenderSystem.cpp
    src/systems/PickingSystem.cpp
)
# stb_image (chargement des textures) : prive, seul GLRenderer s'en sert.
target_include_directories(ee-render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/extern/stb)
target_link_libraries(ee-render PUBLIC
    ee-format
    glad::glad
    glfw
    OpenGL::GL
)

if(NOT EE_BUILD_RUNTIME)
    return()
endif()

add_library(ee-core STATIC
    src/systems/SystemScheduler.cpp
    src/reflect/SceneBind.cpp
)

# ---------------------------------------------------------------------------
# Sidecars des systemes moteur (include/visu/systems/<Nom>.json) : embarques
# dans un header genere, lu par engineSystemInfos(). Les .json restent la
# seule source de verite (signature, priorite), comme pour les systemes du
# projet ; modifier ou ajouter un .json relance la configuration.
# ---------------------------------------------------------------------------
file(GLOB EE_ENGINE_SYSTEM_JSONS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/visu/systems/*.json")
set(EE_ENGINE_SYSTEM_ENTRIES "")
foreach(json_file IN LISTS EE_ENGINE_SYSTEM_JSONS)
    file(READ "${json_file}" json_text)
    string(APPEND EE_ENGINE_SYSTEM_ENTRIES "    R\"EESYS(${json_text})EESYS\",\n")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${json_file}")
endforeach()
file(CONFIGURE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/visu/scene/EngineSystemsData.hpp"
    CONTENT "#pragma once\n\n// Genere par CMake depuis include/visu/systems/*.json -- ne pas editer.\nnamespace ee::scene\n{\n    inline constexpr const char *kEngineSystemJsons[] = {\n${EE_ENGINE_SYSTEM_ENTRIES}    nullptr};\n}\n"
    @ONLY)
target_include_directories(ee-core PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/generated")

add_subdirectory(extern/eliott-ecs)
# SDL ne sert qu'aux manettes (la fenetre et le clavier passent par GLFW) :
# sans son module video, SDL statique n'embarque pas son propre code Wayland,
# qui entre en conflit au link avec celui de GLFW.
set(SDL_VIDEO OFF CACHE BOOL "" FORCE)
set(SDL_UNIX_CONSOLE_BUILD ON CACHE BOOL "" FORCE) # sinon SDL refuse de configurer sans X11/Wayland
add_subdirectory(extern/eliott-input)

target_link_libraries(ee-core PUBLIC
    ee-render
    ee-reflect
    eliott-ecs
    eliott-input
)

# ---------------------------------------------------------------------------
# Outil de regeneration du catalogue de composants du PROJET (jamais dans
# ALL : seul ee-visu le construit explicitement, via "cmake --build --target
# gen_components", pour relire les vrais champs des composants custom via
# Reflect<T> et ecrire Assets/Components.json).
#
# N'existe que si on est embarque dans un VRAI projet etudiant (qui a ce
# fichier, genere par eliott-hub) -- pas quand ee-core est embarque dans
# ee-visu lui-meme, qui n'a pas cette structure de dossiers.
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/Components/RegisterComponents.cpp")
    add_executable(gen_components EXCLUDE_FROM_ALL
        tools/gen_components_main.cpp
        ${CMAKE_SOURCE_DIR}/Components/RegisterComponents.cpp
    )
    target_include_directories(gen_components PRIVATE ${CMAKE_SOURCE_DIR})
    # Le code a lier se limite a la reflexion (ee-reflect). Les headers que
    # peuvent inclure les composants (moteur, ecs, glfw...) viennent des
    # proprietes d'ee-core, lues sans en dependre : rien d'autre n'est compile
    # (a froid : 3 fichiers au lieu du moteur + SDL + glfw).
    target_link_libraries(gen_components PRIVATE ee-reflect)
    target_include_directories(gen_components PRIVATE
        $<TARGET_PROPERTY:ee-core,INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(gen_components PRIVATE
        $<TARGET_PROPERTY:ee-core,INTERFACE_COMPILE_DEFINITIONS>)
    # Au meme niveau que l'executable du jeu (build/<nom>), pas dans le
    # sous-dossier ou ce CMakeLists est imbrique -> sa deduction de racine
    # projet (<build>/..) reste valide quelle que soit la profondeur
    # d'inclusion.
    set_target_properties(gen_components PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
