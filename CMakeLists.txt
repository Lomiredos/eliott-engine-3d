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
    src/runtime/RuntimeSmokeTest.cpp
)

target_include_directories(ee-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/eliott-math/include
)

add_subdirectory(extern/eliott-ecs)
add_subdirectory(extern/eliott-input)

target_link_libraries(ee-core PUBLIC
    glad::glad
    glfw
    OpenGL::GL
    nlohmann_json::nlohmann_json
    eliott-ecs
    eliott-input
)

# ---------------------------------------------------------------------------
# Outil de regeneration du catalogue de composants du PROJET (jamais dans
# ALL : seul ee-visu le construit explicitement, via "cmake --build --target
# gen_components", pour relire les vrais champs des composants custom via
# Reflect<T> et ecrire assets/Components.json).
# ---------------------------------------------------------------------------
add_executable(gen_components EXCLUDE_FROM_ALL
    tools/gen_components_main.cpp
    ${CMAKE_SOURCE_DIR}/Components/RegisterComponents.cpp
)
target_include_directories(gen_components PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(gen_components PRIVATE ee-core)
# Au meme niveau que l'executable du jeu (build/<nom>), pas dans le sous-dossier
# ou ce CMakeLists est imbrique -> sa deduction de racine projet (<build>/..)
# reste valide quelle que soit la profondeur d'inclusion.
set_target_properties(gen_components PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
