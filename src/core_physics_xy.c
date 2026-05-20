#include "core_physics.h"
#include "pcg_random.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TWO_PI (2.0 * M_PI)
#define DELTA_MAX (M_PI / 4.0)
#define IDX(x, y, w) ((y) * (w) + (x))

static inline int wrap(int i, int max)
{
    return (i + max) % max;
}

static inline double wrap_angle(double a)
{
    a = fmod(a, TWO_PI);
    return (a < 0.0) ? a + TWO_PI : a;
}

static inline void hsv_to_rgb(double hue, uint8_t *r, uint8_t *g, uint8_t *b)
{
    double h6 = hue * 6.0;
    int i = (int)h6 % 6;
    double f = h6 - (int)h6;
    double q = 1.0 - f;
    switch (i)
    {
    case 0:
        *r = 255;
        *g = (uint8_t)(f * 255);
        *b = 0;
        break;
    case 1:
        *r = (uint8_t)(q * 255);
        *g = 255;
        *b = 0;
        break;
    case 2:
        *r = 0;
        *g = 255;
        *b = (uint8_t)(f * 255);
        break;
    case 3:
        *r = 0;
        *g = (uint8_t)(q * 255);
        *b = 255;
        break;
    case 4:
        *r = (uint8_t)(f * 255);
        *g = 0;
        *b = 255;
        break;
    case 5:
        *r = 255;
        *g = 0;
        *b = (uint8_t)(q * 255);
        break;
    default:
        *r = 0;
        *g = 0;
        *b = 0;
    }
}

void xy_init_grid(double *grid, int width, int height)
{
    for (int i = 0; i < height * width; i++)
    {
        grid[i] = pcg_rand_double() * TWO_PI;
    }
}

void xy_update_grid(double *grid, int width, int height, double T, double h, double J)
{
    for (int parity = 0; parity <= 1; parity++)
    {
#pragma omp parallel
        {
            uint64_t seed = (uint64_t)time(NULL) ^ ((uint64_t)omp_get_thread_num() * 0x9E3779B97F4A7C15ULL);
            pcg_seed(seed, 42);

#pragma omp for collapse(2) schedule(static)
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    if (((x + y) & 1) != parity)
                        continue;

                    double theta = grid[IDX(x, y, width)];

                    double tL = grid[IDX(wrap(x - 1, width), y, width)];
                    double tR = grid[IDX(wrap(x + 1, width), y, width)];
                    double tU = grid[IDX(x, wrap(y - 1, height), width)];
                    double tD = grid[IDX(x, wrap(y + 1, height), width)];

                    double delta = (pcg_rand_double() * 2.0 - 1.0) * DELTA_MAX;
                    double theta_new = wrap_angle(theta + delta);

                    double dE = -(J * ((cos(theta_new - tL) - cos(theta - tL)) + 
                                       (cos(theta_new - tR) - cos(theta - tR)) + 
                                       (cos(theta_new - tU) - cos(theta - tU)) + 
                                       (cos(theta_new - tD) - cos(theta - tD)))) - h * (cos(theta_new) - cos(theta));

                    if (dE <= 0.0 || (T > 0.0 && pcg_rand_double() < exp(-dE / T)))
                        grid[IDX(x, y, width)] = theta_new;
                }
            }
        }
    }
}

double xy_measure_M(const double *grid, int width, int height)
{
    double sum_x = 0.0;
    double sum_y = 0.0;
#pragma omp parallel for reduction(+:sum_x, sum_y) schedule(static)
    for (int i = 0; i < height * width; i++)
    {
        sum_x += cos(grid[i]);
        sum_y += sin(grid[i]);
    }
    return sqrt(sum_x * sum_x + sum_y * sum_y) / (double)(height * width);
}

void xy_render_pixels(const double *grid, uint32_t *pixels, int width, int height)
{
#pragma omp parallel for collapse(2) schedule(static)
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            double hue = grid[IDX(x, y, width)] / TWO_PI;
            uint8_t r, g, b;
            hsv_to_rgb(hue, &r, &g, &b);
            pixels[y * width + x] = (255u << 24) | (b << 16) | (g << 8) | r;
        }
    }
}
