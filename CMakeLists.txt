cmake_minimum_required(VERSION 3.20)
project(tactics CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED sdl3)
include_directories(include)
include_directories(${SDL3_INCLUDE_DIRS})
include_directories(/usr/include/SDL3_image)
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/types.cpp
    src/animation_loader.cpp
    src/entity.cpp
    src/fx.cpp
    src/timing_loader.cpp
    src/grid_renderer.cpp
    src/perspective.cpp
    src/gpu_renderer.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${SDL3_LIBRARIES}
    SDL3_image
)
