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

add_library(ee-core STATIC
    src/core/SceneInfo.cpp
    src/core/MeshStore.cpp
    src/core/SceneQuery.cpp
    src/core/SystemInfo.cpp
    src/render/GLRenderer.cpp
    src/render/RendererService.cpp
    src/systems/RenderSystem.cpp
    src/systems/PickingSystem.cpp
    src/systems/SystemScheduler.cpp
    src/reflect/CatalogGen.cpp
    src/reflect/SceneBind.cpp
    src/ipc/Socket.cpp
    src/ipc/SharedFrame.cpp
)

target_include_directories(ee-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/eliott-math/include
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
    glad::glad
    glfw
    OpenGL::GL
    nlohmann_json::nlohmann_json
    eliott-ecs
    eliott-input
)

# Lien editeur <-> jeu (visu/ipc) : sockets Winsock sous Windows, shm_open
# (librt) sous Linux.
if(WIN32)
    target_link_libraries(ee-core PUBLIC ws2_32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(ee-core PUBLIC rt)
endif()

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
    target_link_libraries(gen_components PRIVATE ee-core)
    # Au meme niveau que l'executable du jeu (build/<nom>), pas dans le
    # sous-dossier ou ce CMakeLists est imbrique -> sa deduction de racine
    # projet (<build>/..) reste valide quelle que soit la profondeur
    # d'inclusion.
    set_target_properties(gen_components PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
