#include "core_physics.h"
#include "pcg_random.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

#define IDX(x, y, w) ((y) * (w) + (x))

static inline int wrap(int i, int max)
{
    return (i + max) % max;
}

void ising_init_grid(int *grid, int width, int height)
{
    for (int i = 0; i < height * width; i++)
    {
        grid[i] = (pcg_rand_bounded(2) == 0) ? -1 : 1;
    }
}

void ising_update_grid(int *grid, int width, int height, double T, double h, double J)
{
    // Red-Black checkerboard updates using OpenMP
    for (int parity = 0; parity <= 1; parity++)
    {
#pragma omp parallel
        {
            uint64_t seed = (uint64_t)time(NULL) ^ (omp_get_thread_num() * 0x9E3779B97F4A7C15ULL);
            pcg_seed(seed, 42);
#pragma omp for collapse(2) schedule(static)
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    if (((x + y) & 1) != parity)
                        continue;

                    int current = grid[IDX(x, y, width)];
                    int neighbor_sum = grid[IDX(wrap(x - 1, width), y, width)] + 
                                       grid[IDX(wrap(x + 1, width), y, width)] + 
                                       grid[IDX(x, wrap(y - 1, height), width)] + 
                                       grid[IDX(x, wrap(y + 1, height), width)];

                    double delta_E = 2.0 * current * (J * neighbor_sum + h);

                    if (delta_E <= 0.0 || (T > 0.0 && pcg_rand_double() < exp(-delta_E / T)))
                    {
                        grid[IDX(x, y, width)] = -current;
                    }
                }
            }
        }
    }
}

double ising_measure_M(const int *grid, int width, int height)
{
    long long sum = 0;
#pragma omp parallel for reduction(+:sum) schedule(static)
    for (int i = 0; i < height * width; i++)
    {
        sum += grid[i];
    }
    return (double)sum / (double)(height * width);
}

void ising_render_pixels(const int *grid, uint32_t *pixels, int width, int height)
{
#pragma omp parallel for collapse(2) schedule(static)
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            uint8_t r, g, b;
            if (grid[IDX(x, y, width)] == 1)
            {
                r = 255;
                g = 45;   // Fixed octal bug (045 is 37) -> explicitly 45
                b = 1;    // Fixed octal bug (001 is 1)   -> explicitly 1
            }
            else
            {
                r = 73;   // Fixed octal bug (073 is 59) -> explicitly 73
                g = 16;   // Fixed octal bug (016 is 14) -> explicitly 16
                b = 230;
            }
            pixels[y * width + x] = (255u << 24) | (b << 16) | (g << 8) | r;
        }
    }
}
