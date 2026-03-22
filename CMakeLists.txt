cmake_minimum_required(VERSION 3.15)
project(breakout)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ========== 强制所有库静态链接 ==========
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ========== MSVC 运行时库设置 ==========
if(MSVC)
    # Release 使用 /MT，Debug 使用 /MDd
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ========== SFML（源码子目录）==========
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_USE_SYSTEM_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/SFML)

# ========== Box2D ==========
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/box2d)

# ========== OpenGL ==========
find_package(OpenGL REQUIRED)

# ========== ImGui ==========
add_library(imgui STATIC
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
)
target_include_directories(imgui PUBLIC libs/imgui)

# ========== imgui-sfml ==========
add_library(imgui-sfml STATIC libs/imgui-sfml/imgui-sfml.cpp)
target_link_libraries(imgui-sfml
    PRIVATE
    imgui
    sfml-graphics
    sfml-window
    sfml-system
    OpenGL::GL
)
target_include_directories(imgui-sfml PUBLIC libs/imgui libs/imgui-sfml)

# ========== 主程序 ==========
add_executable(breakout "src/breakout.cpp")

# Release 下隐藏控制台（已有）
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_options(breakout PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    target_compile_options(breakout PRIVATE /GL)     # 全程序优化
    target_link_options(breakout PRIVATE /LTCG)      # 链接时代码生成
    target_link_options(breakout PRIVATE /OPT:REF)   # 移除未引用的函数和数据
    target_link_options(breakout PRIVATE /OPT:ICF)   # 合并相同的 COMDAT 段
endif()

# 链接库
target_link_libraries(breakout
    PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    box2d
    imgui-sfml
    OpenGL::GL
)
