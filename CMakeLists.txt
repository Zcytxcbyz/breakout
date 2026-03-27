# CMakeLists.txt for Breakout Game
cmake_minimum_required(VERSION 3.15)
project(breakout)

# ========== C++17 Standard ==========
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========== Default build type is Debug ==========
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ========== Force all libraries to static linking ==========
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ========== MSVC Runtime Library Settings ==========
if(MSVC)
    # Release uses /MT, Debug uses /MDd
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ========== SFML (source subdirectory) ==========
set(SFML_BUILD_AUDIO ON CACHE BOOL "" FORCE)
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

# ========== Main Program ==========
add_executable(breakout "src/breakout.cpp")

# Resource file for Windows
if(WIN32)
    target_sources(breakout PRIVATE "resource.rc")
endif()

# ========== SimpleIni ==========
target_include_directories(breakout PRIVATE libs/simpleini)

# Hide console in Release (already present)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_options(breakout PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    target_compile_options(breakout PRIVATE /GL)     # Whole program optimization
    target_link_options(breakout PRIVATE /LTCG)      # Link-time code generation
    target_link_options(breakout PRIVATE /OPT:REF)   # Remove unreferenced functions and data
    target_link_options(breakout PRIVATE /OPT:ICF)   # Merge identical COMDAT sections
endif()

# Link libraries
target_link_libraries(breakout
    PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
    box2d
    imgui-sfml
    OpenGL::GL
)
