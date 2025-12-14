cmake_minimum_required(VERSION 3.20)
project(DuelystPrototype)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(game
    src/main.cpp
    src/renderer.cpp
    src/asset_manager.cpp
    src/animation_player.cpp
    src/fx_system.cpp
    src/game_state.cpp
    src/texture_atlas.cpp
    src/fx_composition_resolver.cpp
    src/board_renderer.cpp
    src/unit.cpp
    src/shader.cpp
    src/sprite_batch.cpp
)

target_include_directories(game PRIVATE include)

target_link_libraries(game PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    glm::glm
    GLEW::GLEW
    OpenGL::GL
)
