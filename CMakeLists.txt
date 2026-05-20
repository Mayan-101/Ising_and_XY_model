cmake_minimum_required(VERSION 3.12)
project(Kawasaki_Dynamics C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Source files
set(COMMON_SOURCES
    src/config.c
    src/pcg_random.c
    src/plugin_loader.c
    src/core_physics_ising.c
    src/core_physics_xy.c
)

# Find Packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
find_package(SDL2_ttf REQUIRED)
find_package(OpenMP REQUIRED)

# Executable: interactive
add_executable(interactive
    src/main_interactive.c
    src/ui_engine.c
    ${COMMON_SOURCES}
)
target_include_directories(interactive PRIVATE include ${SDL2_INCLUDE_DIRS})
target_link_libraries(interactive
    PRIVATE
    ${SDL2_LIBRARIES}
    SDL2_ttf::SDL2_ttf
    OpenMP::OpenMP_C
    m
)

# Executable: hysteresis
add_executable(hysteresis
    src/main_hysteresis.c
    ${COMMON_SOURCES}
)
target_include_directories(hysteresis PRIVATE include)
target_link_libraries(hysteresis
    PRIVATE
    OpenMP::OpenMP_C
    m
)

# Console configurations for MINGW
if(MINGW)
    set_target_properties(interactive PROPERTIES LINK_FLAGS "-mconsole")
    set_target_properties(hysteresis PROPERTIES LINK_FLAGS "-mconsole")
endif()