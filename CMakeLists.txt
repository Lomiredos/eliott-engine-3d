cmake_minimum_required(VERSION 3.20)
project(ee-core LANGUAGES CXX)

# ---------------------------------------------------------------------------
# ee-core : le coeur moteur 3D partage.
#
# Rendu (contrat IRenderer + backend GLRenderer), systemes (renderScene,
# pickScene) et modele de scene (SceneInfo, MeshStore). Aucune trace d'editeur
# ici (pas d'ImGui) : c'est le code que l'EDITEUR (EE-Visu) et le JEU
# (empty-sphere) linkent tous les deux, chacun avec sa propre toolchain.
#
# On ne fixe NI toolchain NI triplet vcpkg ici : c'est le projet consommateur
# qui le fait (EE-Visu en MinGW, empty-sphere en MSVC). add_subdirectory suffit.
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ee-core STATIC
    src/core/SceneInfo.cpp
    src/core/MeshStore.cpp
    src/core/SceneQuery.cpp
    src/core/SystemInfo.cpp
    src/render/GLRenderer.cpp
    src/systems/RenderSystem.cpp
    src/systems/PickingSystem.cpp
    src/systems/SystemScheduler.cpp
    src/reflect/CatalogGen.cpp
    src/reflect/SceneBind.cpp
    src/input/Input.cpp
)

# PUBLIC : tout consommateur herite de ces include dirs (dont eliott-math).
target_include_directories(ee-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/eliott-math/include
)

find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# PUBLIC : le consommateur recupere automatiquement GL, glad, glfw, json.
target_link_libraries(ee-core PUBLIC
    glad::glad
    glfw
    OpenGL::GL
    nlohmann_json::nlohmann_json
)
