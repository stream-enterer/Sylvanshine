cmake_minimum_required(VERSION 3.20)
project(DuelystPrototype)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Required dependencies:
# Ubuntu/Debian: sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libglm-dev
# Arch: sudo pacman -S sdl2 sdl2_image sdl2_mixer glm
# macOS: brew install sdl2 sdl2_image sdl2_mixer glm

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
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

target_include_directories(game PRIVATE
    include
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
)

target_link_libraries(game PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    OpenGL::GL
)
