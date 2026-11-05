cmake_minimum_required(VERSION 3.20)

project(plant_battlegrounds LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(PB_WARNINGS /W4 /permissive- /utf-8)
else()
    set(PB_WARNINGS -Wall -Wextra)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE)

# rlImGui 的 Raylib_5_5 标签对应 ImGui 1.92.7
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.7
    GIT_SHALLOW TRUE)

FetchContent_Declare(rlimgui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG Raylib_5_5
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(raylib imgui rlimgui)

add_library(imgui_bundle STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)
target_include_directories(imgui_bundle PUBLIC ${imgui_SOURCE_DIR} ${rlimgui_SOURCE_DIR})
target_link_libraries(imgui_bundle PUBLIC raylib)

if(MSVC)
    target_compile_options(imgui_bundle PRIVATE /w)
endif()

add_executable(plant_battlegrounds
    src/main.cpp
    src/core/combat.cpp
    src/game/garden_loop.cpp
    src/game/garden_effects.cpp
    src/game/ai.cpp
    src/game/match.cpp
    src/platform/console.cpp
    src/ui/viewer.cpp
)

target_include_directories(plant_battlegrounds PRIVATE src)
target_compile_options(plant_battlegrounds PRIVATE ${PB_WARNINGS})
target_link_libraries(plant_battlegrounds PRIVATE raylib imgui_bundle)
