#ifndef CORE_PHYSICS_H
#define CORE_PHYSICS_H

#include <stdint.h>

// Ising Model CPU Physics
void ising_init_grid(int *grid, int width, int height);
void ising_update_grid(int *grid, int width, int height, double T, double h, double J);
double ising_measure_M(const int *grid, int width, int height);
void ising_render_pixels(const int *grid, uint32_t *pixels, int width, int height);

// XY Model CPU Physics
void xy_init_grid(double *grid, int width, int height);
void xy_update_grid(double *grid, int width, int height, double T, double h, double J);
double xy_measure_M(const double *grid, int width, int height);
void xy_render_pixels(const double *grid, uint32_t *pixels, int width, int height);

#endif
