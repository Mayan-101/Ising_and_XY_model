cmake_minimum_required(VERSION 3.17)
project(XY_or_Ising_Model C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configure output directories so all executables are placed in the build folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

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

# Post-build copy operations for C executables
add_custom_command(TARGET interactive POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:interactive> ${CMAKE_CURRENT_SOURCE_DIR}/Ising.exe
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:interactive> ${CMAKE_CURRENT_SOURCE_DIR}/xy_interactive.exe
    COMMENT "Copying interactive executables to root for backward compatibility"
)

add_custom_command(TARGET hysteresis POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:hysteresis> ${CMAKE_CURRENT_SOURCE_DIR}/ising_hysteresis.exe
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:hysteresis> ${CMAKE_CURRENT_SOURCE_DIR}/ising_hysteresis_gpu.exe
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:hysteresis> ${CMAKE_CURRENT_SOURCE_DIR}/xy_hysteresis_gpu.exe
    COMMENT "Copying hysteresis executables to root for backward compatibility"
)

# Locate NVCC and vcvarsall.bat to compile GPU CUDA DLLs on Windows
find_program(CUDA_NVCC nvcc
    HINTS "$ENV{CUDA_PATH}/bin"
    PATHS
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.8/bin"
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.7/bin"
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6/bin"
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.5/bin"
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.4/bin"
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.3/bin"
)

set(VCVARS_PATHS
    "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvarsall.bat"
    "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvarsall.bat"
    "C:/Program Files (x86)/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvarsall.bat"
    "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvarsall.bat"
    "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Auxiliary/Build/vcvarsall.bat"
)

set(VCVARS_PATH "")
foreach(path ${VCVARS_PATHS})
    if(EXISTS "${path}")
        set(VCVARS_PATH "${path}")
        break()
    endif()
endforeach()

# Only build GPU plugins if both NVCC and MSVC environment wrapper are found
if(CUDA_NVCC AND VCVARS_PATH)
    message(STATUS "CUDA and MSVC environment found. GPU DLLs will be built automatically.")
    
    # Generate helper batch files to compile GPU DLLs under MSVC shell environment
    file(WRITE ${CMAKE_BINARY_DIR}/build_ising.bat
        "@echo off\n"
        "call \"${VCVARS_PATH}\" x64\n"
        "\"${CUDA_NVCC}\" -o \"${CMAKE_BINARY_DIR}/ising.dll\" --shared \"${CMAKE_CURRENT_SOURCE_DIR}/src_gpu/ising_gpu.cu\" -arch=sm_61 -lcurand -Wno-deprecated-gpu-targets --compiler-options /O2,/MT -Xlinker /DEFAULTLIB:Version.lib -Xlinker /DEFAULTLIB:libcmt.lib\n"
    )

    file(WRITE ${CMAKE_BINARY_DIR}/build_xy_gpu.bat
        "@echo off\n"
        "call \"${VCVARS_PATH}\" x64\n"
        "\"${CUDA_NVCC}\" -o \"${CMAKE_BINARY_DIR}/xy_gpu.dll\" --shared \"${CMAKE_CURRENT_SOURCE_DIR}/src_gpu/xy_gpu.cu\" -arch=sm_61 -lcurand -Wno-deprecated-gpu-targets --compiler-options /O2,/MT -Xlinker /DEFAULTLIB:Version.lib -Xlinker /DEFAULTLIB:libcmt.lib\n"
    )
    
    # 1. Ising GPU DLL target
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/ising.dll
        COMMAND ${CMAKE_BINARY_DIR}/build_ising.bat
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/ising.dll ${CMAKE_CURRENT_SOURCE_DIR}/ising.dll
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src_gpu/ising_gpu.cu
        COMMENT "Compiling and copying ising.dll using helper batch script"
    )
    add_custom_target(ising_dll ALL DEPENDS ${CMAKE_BINARY_DIR}/ising.dll)

    # 2. XY GPU DLL target
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/xy_gpu.dll
        COMMAND ${CMAKE_BINARY_DIR}/build_xy_gpu.bat
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/xy_gpu.dll ${CMAKE_CURRENT_SOURCE_DIR}/xy_gpu.dll
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src_gpu/xy_gpu.cu
        COMMENT "Compiling and copying xy_gpu.dll using helper batch script"
    )
    add_custom_target(xy_gpu_dll ALL DEPENDS ${CMAKE_BINARY_DIR}/xy_gpu.dll)
else()
    if(NOT CUDA_NVCC)
        message(WARNING "nvcc not found. GPU DLL compilation will be skipped.")
    endif()
    if(NOT VCVARS_PATH)
        message(WARNING "vcvarsall.bat not found. GPU DLL compilation will be skipped.")
    endif()
endif()