@echo off
setlocal
cd /d "%~dp0.."

echo --- Creating Build Directory ---
if not exist build mkdir build

echo.
echo --- Building GPU Plugins (CUDA) ---
set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
if not exist %VCVARS% (
    set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
)

if not exist %VCVARS% (
    echo WARNING: vcvarsall.bat not found. DLL compilation might be skipped.
) else (
    call %VCVARS% x64
    set CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.8
    
    echo Building xy_gpu.dll...
    "%CUDA_PATH%\bin\nvcc.exe" -o build\xy_gpu.dll --shared src_gpu\xy_gpu.cu -arch=sm_61 -lcurand -Wno-deprecated-gpu-targets --compiler-options /O2,/MT -Xlinker /DEFAULTLIB:Version.lib -Xlinker /DEFAULTLIB:libcmt.lib
    
    echo Building ising.dll...
    "%CUDA_PATH%\bin\nvcc.exe" -o build\ising.dll --shared src_gpu\ising_gpu.cu -arch=sm_61 -lcurand -Wno-deprecated-gpu-targets --compiler-options /O2,/MT -Xlinker /DEFAULTLIB:Version.lib -Xlinker /DEFAULTLIB:libcmt.lib
)

echo.
echo --- Building C Executables with GCC ---
set GCC=C:\msys64\ucrt64\bin\gcc.exe
set SDL_INC=C:\msys64\ucrt64\include\SDL2
set SDL_LIB=C:\msys64\ucrt64\lib

if not exist "%GCC%" (
    echo ERROR: GCC not found at %GCC%
    exit /b 1
)

set COMMON_SRCS=src\config.c src\pcg_random.c src\plugin_loader.c src\core_physics_ising.c src\core_physics_xy.c

echo Building interactive.exe...
"%GCC%" src\main_interactive.c src\ui_engine.c %COMMON_SRCS% -Iinclude -I"%SDL_INC%" -L"%SDL_LIB%" -lSDL2 -lSDL2_ttf -lm -fopenmp -O2 -std=c11 -mconsole -o build\interactive.exe

echo Building hysteresis.exe...
"%GCC%" src\main_hysteresis.c %COMMON_SRCS% -Iinclude -lm -fopenmp -O2 -std=c11 -o build\hysteresis.exe

echo.
echo --- Copying DLLs and Exes for backward compatibility ---
if exist build\xy_gpu.dll (
    copy build\xy_gpu.dll build\.. >nul
    copy build\xy_gpu.dll . >nul
)
if exist build\ising.dll (
    copy build\ising.dll build\.. >nul
    copy build\ising.dll . >nul
)

if exist build\interactive.exe (
    copy build\interactive.exe Ising.exe >nul
    copy build\interactive.exe xy_interactive.exe >nul
    copy build\interactive.exe build\xy_interactive.exe >nul
)

if exist build\hysteresis.exe (
    copy build\hysteresis.exe ising_hysteresis.exe >nul
    copy build\hysteresis.exe ising_hysteresis_gpu.exe >nul
    copy build\hysteresis.exe xy_hysteresis_gpu.exe >nul
)

echo.
echo --- Build Complete ---
