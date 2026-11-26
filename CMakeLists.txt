cmake_minimum_required(VERSION 3.21)
project(TavernGamesVisual VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        v1.3.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(minhook)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stb)

add_library(imgui_static STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)
target_include_directories(imgui_static PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_static PUBLIC d3d11 dxgi)

add_library(TavernGamesVisual SHARED
    src/skse_min.h
    src/assets.h
    src/assets.cpp
    src/games.h
    src/games.cpp
    src/hooks.h
    src/hooks.cpp
    src/main.cpp
)

target_include_directories(TavernGamesVisual PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${stb_SOURCE_DIR}
)

target_link_libraries(TavernGamesVisual PRIVATE
    imgui_static
    minhook
    d3d11
    dxgi
    dinput8
    dxguid
    winmm
    user32
    gdi32
    shell32
    ole32
)

if (MSVC)
    target_compile_options(TavernGamesVisual PRIVATE /utf-8 /W3 /EHsc /bigobj)
    target_compile_definitions(TavernGamesVisual PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        DIRECTINPUT_VERSION=0x0800
        _CRT_SECURE_NO_WARNINGS
    )
    set_property(TARGET TavernGamesVisual PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET imgui_static PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET minhook PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set_target_properties(TavernGamesVisual PROPERTIES
    OUTPUT_NAME "TavernGamesVisual"
    PREFIX ""
)
