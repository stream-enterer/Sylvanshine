cmake_minimum_required(VERSION 3.20)
project(tactics CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Assets are now loaded from dist/ directory (built by build_assets.py)
# DUELYST_REPO_PATH is no longer required at runtime

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED sdl3)
include_directories(include)
include_directories(third_party)
include_directories(${SDL3_INCLUDE_DIRS})
include_directories(/usr/include/SDL3_image)
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/types.cpp
    src/animation_loader.cpp
    src/plist_parser.cpp
    src/asset_manager.cpp
    src/entity.cpp
    src/fx.cpp
    src/grid_renderer.cpp
    src/perspective.cpp
    src/gpu_renderer.cpp
    src/render_pass.cpp
    src/text_renderer.cpp
    src/lighting_presets.cpp
    src/settings_menu.cpp
    src/game_logic.cpp
    src/input.cpp
    src/scene_render.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL3_LIBRARIES}
    SDL3_image
)
