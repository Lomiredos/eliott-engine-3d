cmake_minimum_required(VERSION 3.20)
project(ee-core LANGUAGES CXX)

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

target_include_directories(ee-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/eliott-math/include
)

find_package(OpenGL REQUIRED)

target_link_libraries(ee-core PUBLIC
    glad::glad
    glfw
    OpenGL::GL
    nlohmann_json::nlohmann_json
)
