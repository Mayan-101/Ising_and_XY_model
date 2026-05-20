# Usage Guide

This document outlines how to build, configure, run, and visualize the simulations in the XY or Ising Model Simulation Suite.

---

## 1. Compilation
The project build system is managed fully by CMake. All C executables and CUDA GPU DLLs are compiled automatically.

To compile:
1. Open your terminal (e.g., PowerShell or Command Prompt).
2. Configure the build directory using the MinGW GCC compiler generator:
   ```powershell
   cmake -G "MinGW Makefiles" -B build
   ```
3. Compile the entire project:
   ```powershell
   cmake --build build
   ```

*Upon successful compilation, CMake automatically copies the compiled executables (`Ising.exe`, `ising_hysteresis.exe`, etc.) and the GPU plugins (`ising.dll`, `xy_gpu.dll`) directly to the root directory for execution.*

---

## 2. Running the Simulation Suite

### Option A: The Interactive Menu (Recommended)
An interactive terminal menu allows you to easily configure parameters and launch any run configuration (CPU or GPU, Interactive Graphics or Batch Hysteresis).

To run the menu:
```powershell
.\scripts\run_all.bat
```

### Option B: Command Line Run (Direct Execution)
You can launch the compiled executables directly with custom parameters.

#### 1. Interactive Graphic Modes (SDL2 UI)
Runs the simulation in a live visual window. Key bindings: `[Up/Down]` controls temperature $T$, `[Left/Right]` controls magnetic field $h$.
* **Ising Model (Preferred GPU):** `.\Ising.exe --gpu` (falls back to CPU if GPU DLL is missing)
* **Ising Model (CPU Only):** `.\Ising.exe --cpu`
* **XY Model (Preferred GPU):** `.\xy_interactive.exe --gpu`
* **XY Model (CPU Only):** `.\xy_interactive.exe --cpu`

#### 2. Hysteresis Sweep Batch Modes
Runs a batch parameter sweep and outputs a CSV file.
```powershell
# Usage format:
# <executable> [mode] [T] [h_max] [h_steps] [equil_sweeps] [meas_sweeps] [J]
```
* **Ising Model Hysteresis Sweep:**
  ```powershell
  .\ising_hysteresis_gpu.exe --gpu 1.5 3.0 300 500 200 1.0
  ```
* **XY Model Hysteresis Sweep:**
  ```powershell
  .\xy_hysteresis_gpu.exe --gpu 0.89 3.0 300 500 200 1.0
  ```

---

## 3. Visualizing Results
After running a Hysteresis Sweep Batch Mode, the simulation outputs a CSV dataset (e.g., `ising_hysteresis.csv` or `xy_hysteresis.csv`). You can generate high-quality plots of the hysteresis loops automatically:

1. Select **Option 9 ("Plot Sweep Results")** from the interactive menu (`run_all.bat`).
2. Alternatively, run the Python plotting script directly:
   ```powershell
   python plotter/plot_hysteresis.py
   ```

The script will search for CSV files in the folder and output beautiful visualization plots (such as `plotter/ising_plot.png` and `plotter/xy_plot.png`).
