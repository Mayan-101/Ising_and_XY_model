#include "config.h"
#include "core_physics.h"
#include "plugin_loader.h"
#include "pcg_random.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>

void run_hysteresis_cpu(const SimConfig* cfg, const char* out_filename) {
    FILE *fp = fopen(out_filename, "w");
    if (!fp) { fprintf(stderr, "Cannot open %s for writing\n", out_filename); return; }
    fprintf(fp, "h,M,branch\n");

    printf("Running hysteresis sweep on CPU... (Output: %s)\n", out_filename);
    printf("T=%.4f  h_max=%.4f  h_steps=%d  equil=%d  meas=%d  J=%.4f\n",
           cfg->default_T, cfg->h_max, cfg->h_steps, cfg->equil_sweeps, cfg->meas_sweeps, cfg->J);

    void* grid = NULL;
    if (cfg->is_ising) {
        grid = malloc(cfg->width * cfg->height * sizeof(int));
        if (!grid) { fclose(fp); return; }
        // Initialize all spins pointing up (M = 1.0)
        for (int i = 0; i < cfg->width * cfg->height; i++) ((int*)grid)[i] = 1;
    } else {
        grid = malloc(cfg->width * cfg->height * sizeof(double));
        if (!grid) { fclose(fp); return; }
        // Initialize all spins pointing up (theta = 0.0)
        for (int i = 0; i < cfg->width * cfg->height; i++) ((double*)grid)[i] = 0.0;
    }

    // Branch 0 (descending)
    printf("Branch 0 (descending): ");
    fflush(stdout);
    for (int step = 0; step <= cfg->h_steps; step++) {
        double h = cfg->h_max - (2.0 * cfg->h_max * step) / cfg->h_steps;

        // Equilibrate
        for (int s = 0; s < cfg->equil_sweeps; s++) {
            if (cfg->is_ising) {
                ising_update_grid((int*)grid, cfg->width, cfg->height, cfg->default_T, h, cfg->J);
            } else {
                xy_update_grid((double*)grid, cfg->width, cfg->height, cfg->default_T, h, cfg->J);
            }
        }

        // Measure
        double M_acc = 0.0;
        for (int s = 0; s < cfg->meas_sweeps; s++) {
            if (cfg->is_ising) {
                ising_update_grid((int*)grid, cfg->width, cfg->height, cfg->default_T, h, cfg->J);
                M_acc += ising_measure_M((const int*)grid, cfg->width, cfg->height);
            } else {
                xy_update_grid((double*)grid, cfg->width, cfg->height, cfg->default_T, h, cfg->J);
                M_acc += xy_measure_M((const double*)grid, cfg->width, cfg->height);
            }
        }

        fprintf(fp, "%.8f,%.8f,0\n", h, M_acc / cfg->meas_sweeps);

        if (step % (cfg->h_steps / 10 + 1) == 0) {
            printf("%.0f%% ", 100.0 * step / cfg->h_steps);
            fflush(stdout);
        }
    }
    printf("100%% done\n");

    // Branch 1 (ascending)
    printf("Branch 1 (ascending):  ");
    fflush(stdout);
    for (int step = 0; step <= cfg->h_steps; step++) {
        double h = -cfg->h_max + (2.0 * cfg->h_max * step) / cfg->h_steps;

        // Equilibrate
        for (int s = 0; s < cfg->equil_sweeps; s++) {
            if (cfg->is_ising) {
                ising_update_grid((int*)grid, cfg->width, cfg->height, cfg->default_T, h, cfg->J);
            } else {
                xy_update_grid((double*)grid, cfg->width, cfg->height, cfg->default_T, h, cfg->J);
            }
        }

        // Measure
        double M_acc = 0.0;
        for (int s = 0; s < cfg->meas_sweeps; s++) {
            if (cfg->is_ising) {
                ising_update_grid((int*)grid, cfg->width, cfg->height, cfg->default_T, h, cfg->J);
                M_acc += ising_measure_M((const int*)grid, cfg->width, cfg->height);
            } else {
                xy_update_grid((double*)grid, cfg->width, cfg->height, cfg->default_T, h, cfg->J);
                M_acc += xy_measure_M((const double*)grid, cfg->width, cfg->height);
            }
        }

        fprintf(fp, "%.8f,%.8f,1\n", h, M_acc / cfg->meas_sweeps);

        if (step % (cfg->h_steps / 10 + 1) == 0) {
            printf("%.0f%% ", 100.0 * step / cfg->h_steps);
            fflush(stdout);
        }
    }
    printf("100%% done\n");

    fclose(fp);
    free(grid);
    printf("Saved CPU hysteresis data to %s\n", out_filename);
}

int main(int argc, char** argv) {
    SimConfig cfg;
    init_config(&cfg, argc, argv);

#pragma omp parallel
    {
        uint64_t seed = (uint64_t)time(NULL) ^ ((uint64_t)omp_get_thread_num() * 0x9E3779B97F4A7C15ULL);
        pcg_seed(seed, 42);
    }
    pcg_seed((uint64_t)time(NULL), 42);

#pragma omp parallel
    {
#pragma omp single
        printf("Running with %d OpenMP threads\n", omp_get_num_threads());
    }

    const char* out_filename = cfg.is_ising ? "ising_hysteresis.csv" : "xy_hysteresis.csv";

    SimulationPlugin gpu = {0};
    bool use_gpu = false;
    if (cfg.use_gpu) {
        const char* dll_name = cfg.is_ising ? "ising.dll" : "xy_gpu.dll";
        use_gpu = plugin_load(&gpu, dll_name);
        if (!use_gpu) {
            char alt_path[256];
            snprintf(alt_path, sizeof(alt_path), "build/%s", dll_name);
            use_gpu = plugin_load(&gpu, alt_path);
        }
    }

    if (use_gpu) {
        printf("Running hysteresis sweep on GPU... (Output: %s)\n", out_filename);
        printf("T=%.4f  h_max=%.4f  h_steps=%d  equil=%d  meas=%d  J=%.4f\n",
               cfg.default_T, cfg.h_max, cfg.h_steps, cfg.equil_sweeps, cfg.meas_sweeps, cfg.J);
        
        void* gpu_state = gpu.init(cfg.height, cfg.width);
        
        // Initialize grid pointing up on GPU
        if (cfg.is_ising) {
            int* host_grid = (int*)malloc(cfg.width * cfg.height * sizeof(int));
            for (int i = 0; i < cfg.width * cfg.height; i++) host_grid[i] = 1;
            gpu.upload_grid(gpu_state, host_grid);
            free(host_grid);
        } else {
            double* host_grid = (double*)malloc(cfg.width * cfg.height * sizeof(double));
            for (int i = 0; i < cfg.width * cfg.height; i++) host_grid[i] = 0.0;
            gpu.upload_grid(gpu_state, host_grid);
            free(host_grid);
        }

        gpu.run_hysteresis(gpu_state, cfg.default_T, cfg.h_max, cfg.h_steps, cfg.equil_sweeps, cfg.meas_sweeps, cfg.J);
        gpu.save_csv(gpu_state, out_filename);
        gpu.destroy(gpu_state);
        plugin_unload(&gpu);
        printf("Saved GPU hysteresis data to %s\n", out_filename);
    } else {
        run_hysteresis_cpu(&cfg, out_filename);
    }

    return 0;
}
