# Parameter Explanation

This document provides a brief technical explanation of the core parameters that govern the accuracy, statistical reliability, and resolution of the XY or Ising Model hysteresis sweeps.

---

## 1. `h_steps` (Field Steps / Sweep Resolution)
* **Code Identifier:** `h_steps` (parsed as `cfg->h_steps`)
* **Interactive Menu Label:** `3. Hysteresis Steps`

### Technical Description
`h_steps` defines the **resolution of the external magnetic field sweep**. It represents the number of discrete steps the simulation takes as it adjusts the external magnetic field $h$ from $+h_{max}$ down to $-h_{max}$ (Descending Branch / Branch 0) and back up from $-h_{max}$ to $+h_{max}$ (Ascending Branch / Branch 1).

* **Low Values (e.g., 50):** Runs very fast, but yields a blocky, low-resolution loop with sparse data points.
* **High Values (e.g., 300+):** Produces a highly detailed, smooth hysteresis curve, capturing fine details of Phase transitions and domain-wall pinning, but increases total simulation time linearly.

---

## 2. `eq_steps` (Equilibration Sweeps)
* **Code Identifier:** `equil_sweeps` (parsed as `cfg->equil_sweeps`)
* **Interactive Menu Label:** `4. Equilibration Sweeps`

### Technical Description
`eq_steps` specifies the number of **non-measurement Monte Carlo sweeps** performed at the start of *every* field step. A single sweep corresponds to visiting every lattice site exactly once (via checkerboard updates).

When the external magnetic field $h$ changes, the spin system is suddenly pushed out of thermodynamic equilibrium. The system needs time to relax and thermalize under the new field strength. 
* During equilibration sweeps, the lattice updates are run but **no measurements are recorded**.
* This allows the system to shed its transient states and settle into the true steady state for that particular field strength, eliminating historical starting bias from the previous step.
* Failing to provide enough equilibration sweeps leads to an unphysical lag in the magnetization response (artificial widening of the hysteresis loop).

---

## 3. `mea_steps` (Measurement Sweeps)
* **Code Identifier:** `meas_sweeps` (parsed as `cfg->meas_sweeps`)
* **Interactive Menu Label:** `5. Measurement Sweeps`

### Technical Description
`mea_steps` represents the number of **measurement Monte Carlo sweeps** performed *after* the equilibration phase has completed at each field step.

Because Monte Carlo simulations are stochastic (probabilistic) processes governed by thermal fluctuations, any single snapshot of the lattice has high statistical noise. To obtain a physically accurate value for macroscopic observables (such as the net magnetization $M$):
1. The simulation performs `mea_steps` consecutive sweeps.
2. At the end of each sweep, it measures the net magnetization $M = \frac{1}{N}\sum s_i$.
3. It accumulates these values and takes the average:
   $$M_{avg} = \frac{1}{\text{mea\_steps}} \sum_{t=1}^{\text{mea\_steps}} M(t)$$

* **Low Values (e.g., 50):** Decreases computation time, but results in high statistical variance (noisy plots) due to thermal fluctuations.
* **High Values (e.g., 200+):** Averages out the thermal fluctuations (statistical noise), producing highly reliable, smooth, and statistically sound physical measurements.
