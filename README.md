# XY or Ising Model Simulation Suite

An interactive, high-performance simulation suite for two classical spin models from statistical mechanics — the **2D Ising Model** and the **2D XY Model** — rendered live at 800×800 pixels using SDL2. 

The suite features a multi-threaded CPU physics engine (parallelized via OpenMP) and an optional GPU backend compiled dynamically as a CUDA DLL for massively parallel execution. It includes interactive graphical simulation modes, automated magnetic hysteresis parameter sweeps, Python plotters, and a premium command-line dashboard interface.

---

## Demo Preview

![XY or Ising Model Simulation Live Demo](https://github.com/user-attachments/assets/d8a6e84b-8eee-4586-8cdd-15600579f198)

---

## Mathematical Foundations & Physical Transitions

### 1. The 2D Ising Model
The 2D Ising model places a discrete spin $s_i \in \{-1, +1\}$ on every site $i$ of a square lattice. The configuration energy is described by the Hamiltonian:

$$H = -J \sum_{\langle i,j \rangle} s_i s_j - h \sum_i s_i$$

where $\langle i,j \rangle$ represents nearest-neighbor lattice pairs, $J$ is the coupling constant, and $h$ is the external magnetic field.
* **Critical Phase Transition:** Spontaneously magnetizes below Onsager's critical temperature:
  $$T_c = \frac{2J}{\ln(1 + \sqrt{2})} \approx 2.269J$$
* **Visuals:** Below $T_c$, spins align into solid domains. Above $T_c$, thermal fluctuations dissolve order into high-entropy noise. At $T_c$, boundaries form infinite fractal structures.

### 2. The 2D XY Model
The XY model generalizes the spin state to a continuous planar angle $\theta_i \in [0, 2\pi)$. The configuration energy is:

$$H = -J \sum_{\langle i,j \rangle} \cos(\theta_i - \theta_j) - h \sum_i \cos(\theta_i)$$

* **Berezinskii–Kosterlitz–Thouless (BKT) Transition:** Spontaneous symmetry breaking is forbidden in 2D continuous systems (Mermin–Wagner theorem). Instead, a topological transition occurs at $T_{BKT} \approx 0.89J$ driven by the pairing/unbinding of **topological vortices**.
* **Visuals:** Angle $\theta_i$ maps to HSV color space. Aligned domains appear as solid color fields, continuous spin textures form smooth gradients, and color singularities mark the cores of topological vortices.

---

## Parallelism: Checkerboard Decomposition

To prevent race conditions during parallel updates (where adjacent threads attempt to update neighboring spins simultaneously, leading to unphysical energy calculations), the suite uses a **red-black (checkerboard) decomposition**:
1. The lattice is divided into two interlocking sublattices by parity: Even $(x + y \text{ mod } 2 = 0)$ and Odd $(x + y \text{ mod } 2 = 1)$.
2. **Pass 1:** Update all even sites in parallel (no two even sites share a neighbor).
3. **Pass 2:** Update all odd sites in parallel (no two odd sites share a neighbor).

This guarantees absolute data-parallelism with zero memory locks, implemented identically in CPU OpenMP loops and GPU CUDA kernels.

---

## Project Directory Layout
The codebase is refactored from legacy, tightly coupled scripts into a highly cohesive, modular architecture:

```text
.
├── CMakeLists.txt         # Unified build system (handles C/MSYS2 & NVCC/MSVC)
├── README.md              # Project introduction
├── assets/                
│   └── font.ttf           # Default font asset bundled for UI HUD rendering
├── docs/                  # Detailed documentation manuals
│   ├── Usage.md           # How to compile, run, and plot
│   └── explanation.md     # Mathematical & technical parameter explanations
├── include/               # Public C header files (config, physics, ui, plugins)
├── plotter/               # Python plotting utilities and virtual environments
│   └── plot_hysteresis.py # Matplotlib plotter for generated hysteresis CSVs
├── scripts/               
│   └── run_all.bat        # Interactive terminal console dashboard (TUI)
├── src/                   # Core C source files (config, physics, main, ui)
└── src_gpu/               # CUDA GPU kernel source files (Ising & XY)
```

---

## Quick Start: Building & Running

### Prerequisites
* **MSYS2 (MinGW UCRT64):** Standard GCC compiler environment.
* **CMake (v3.17+):** Project configuration.
* **SDL2 & SDL2_ttf:** Graphics library packages installed via MSYS2.
* **CUDA Toolkit (v12.x) & MSVC Build Tools:** Required to compile the GPU kernels.

### 1. Compile Natively via CMake
You can configure and build the entire project natively. The CMake build system compiles the CPU targets using GCC and compiles the CUDA targets using NVCC under the Visual Studio MSVC context automatically:

```powershell
# Configure the build directory
cmake -G "MinGW Makefiles" -B build

# Compile the suite (all DLLs and EXEs will be copied to the root folder)
cmake --build build
```

### 2. Launch the Suite
Simply execute the unified console dashboard to configure simulation parameters and launch interactive graphics or batch sweeps:
```powershell
.\scripts\run_all.bat
```

---

## Dynamic HUD & Interactive Controls
When running the interactive modes (`Ising.exe` or `xy_interactive.exe`), you are presented with a real-time graphics canvas. The drop-shadowed HUD in the top-left displays the simulation's current state:

| Control Binding | Action |
|---|---|
| `↑` / `↓` | Increase / Decrease Temperature $T$ |
| `←` / `→` | Decrease / Increase External Magnetic Field $h$ |
| **HUD Fields** | Real-time display of $T$, $h$ (or $B$), and Coupling Constant $J$ |

---

## Further Reading
For extensive manuals, refer to the `docs/` folder:
* **[docs/Usage.md](file:///c:/Users/mayan/Kawasaki_Dynamics/docs/Usage.md):** Complete guide on parameters, command-line arguments, plotting, and direct binary configurations.
* **[docs/explanation.md](file:///c:/Users/mayan/Kawasaki_Dynamics/docs/explanation.md):** Deep-dive technical explanation of numerical parameters including `h_steps`, `eq_steps` (equilibration sweeps), and `mea_steps` (measurement sweeps).
