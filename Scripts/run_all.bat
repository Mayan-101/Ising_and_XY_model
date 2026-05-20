@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0.."

:menu
cls
echo =======================================================================
echo          KAWASAKI DYNAMICS SIMULATION SUITE - INTERACTIVE MENU
echo =======================================================================
echo.
echo   [SDL2 INTERACTIVE GRAPHICS MODE]
echo     1. Ising Model  - GPU accelerated, falls back to CPU
echo     2. Ising Model  - CPU Only
echo     3. XY Model     - GPU accelerated, falls back to CPU
echo     4. XY Model     - CPU Only
echo.
echo   [HYSTERESIS SWEEP BATCH MODE]
echo     5. Ising Hysteresis Sweep - GPU accelerated, falls back to CPU
echo     6. Ising Hysteresis Sweep - CPU Only
echo     7. XY Hysteresis Sweep    - GPU accelerated, falls back to CPU
echo     8. XY Hysteresis Sweep    - CPU Only
echo.
echo   [ANALYSIS / VISUALIZATION]
echo     9. Plot Sweep Results
echo.
echo   [EXIT]
echo     10. Exit
echo.
echo =======================================================================
set /p choice="Select an option [1-10]: "

if "%choice%"=="1" goto opt1
if "%choice%"=="2" goto opt2
if "%choice%"=="3" goto opt3
if "%choice%"=="4" goto opt4
if "%choice%"=="5" goto opt5
if "%choice%"=="6" goto opt6
if "%choice%"=="7" goto opt7
if "%choice%"=="8" goto opt8
if "%choice%"=="9" goto opt9
if "%choice%"=="10" goto opt10

echo Invalid choice. Please try again.
pause
goto menu

:opt1
echo.
echo Launching Interactive Ising - GPU preferred...
Ising.exe --gpu
pause
goto menu

:opt2
echo.
echo Launching Interactive Ising - CPU Only...
Ising.exe --cpu
pause
goto menu

:opt3
echo.
echo Launching Interactive XY Model - GPU preferred...
xy_interactive.exe --gpu
pause
goto menu

:opt4
echo.
echo Launching Interactive XY Model - CPU Only...
xy_interactive.exe --cpu
pause
goto menu

:opt5
set "MODEL=Ising"
set "EXE=ising_hysteresis_gpu.exe"
set "DEFAULT_T=1.5"
set "FORCE_CPU="
goto configure_sweep

:opt6
set "MODEL=Ising"
set "EXE=ising_hysteresis.exe"
set "DEFAULT_T=1.5"
set "FORCE_CPU=--cpu"
goto configure_sweep

:opt7
set "MODEL=XY"
set "EXE=xy_hysteresis_gpu.exe"
set "DEFAULT_T=0.89"
set "FORCE_CPU="
goto configure_sweep

:opt8
set "MODEL=XY"
set "EXE=xy_hysteresis_gpu.exe"
set "DEFAULT_T=0.89"
set "FORCE_CPU=--cpu"
goto configure_sweep

:opt9
echo.
echo Generating plots using Python virtual environment...
if exist plotter\venv\Scripts\python.exe (
    plotter\venv\Scripts\python.exe plotter\plot_hysteresis.py
) else (
    echo Warning: plotter\venv\Scripts\python.exe not found. Falling back to system python...
    python plotter\plot_hysteresis.py
)
pause
goto menu

:opt10
echo Goodbye!
exit /b 0

:configure_sweep
cls
echo =======================================================================
echo                  SWEEP PARAMETER CONFIGURATION - !MODEL!
echo =======================================================================
echo.
echo Leave blank and press [Enter] to accept the recommended defaults.
echo.
set "T=!DEFAULT_T!"
set "h_max=3.0"
set "h_steps=300"
set "equil=500"
set "meas=200"
set "J=1.0"

set /p user_T="1. Temperature T [default !T!]: "
if not "!user_T!"=="" set "T=!user_T!"

set /p user_h="2. Max Field h_max [default !h_max!]: "
if not "!user_h!"=="" set "h_max=!user_h!"

set /p user_steps="3. Hysteresis Steps [default !h_steps!]: "
if not "!user_steps!"=="" set "h_steps=!user_steps!"

set /p user_equil="4. Equilibration Sweeps [default !equil!]: "
if not "!user_equil!"=="" set "equil=!user_equil!"

set /p user_meas="5. Measurement Sweeps [default !meas!]: "
if not "!user_meas!"=="" set "meas=!user_meas!"

set /p user_J="6. Coupling Constant J [default !J!]: "
if not "!user_J!"=="" set "J=!user_J!"

echo.
echo Launching: !EXE! !FORCE_CPU! !T! !h_max! !h_steps! !equil! !meas! !J!
echo.
!EXE! !FORCE_CPU! !T! !h_max! !h_steps! !equil! !meas! !J!
set "FORCE_CPU="
echo.
echo Sweep completed.
pause
goto menu
