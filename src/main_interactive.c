#include "config.h"
#include "core_physics.h"
#include "plugin_loader.h"
#include "ui_engine.h"
#include "pcg_random.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

int main(int argc, char** argv) {
    SimConfig cfg;
    init_config(&cfg, argc, argv);

    // Initialise OpenMP thread-private seeds
#pragma omp parallel
    {
        uint64_t seed = (uint64_t)time(NULL) ^ ((uint64_t)omp_get_thread_num() * 0x9E3779B97F4A7C15ULL);
        pcg_seed(seed, 42);
    }
    // Main thread seed
    pcg_seed((uint64_t)time(NULL), 42);

    printf("Starting Interactive Simulation\n");
    printf("Model: %s\n", cfg.is_ising ? "Ising" : "XY");
    printf("Dimensions: %d x %d\n", cfg.width, cfg.height);

    SimulationPlugin gpu = {0};
    bool use_gpu = false;
    if (cfg.use_gpu) {
        const char* dll_name = cfg.is_ising ? "ising.dll" : "xy_gpu.dll";
        use_gpu = plugin_load(&gpu, dll_name);
        if (!use_gpu) {
            // Try build subdirectory
            char alt_path[256];
            snprintf(alt_path, sizeof(alt_path), "build/%s", dll_name);
            use_gpu = plugin_load(&gpu, alt_path);
        }
        if (use_gpu) {
            printf("Running simulation on GPU.\n");
        } else {
            printf("GPU library loading failed. Falling back to CPU.\n");
        }
    } else {
        printf("Forcing CPU mode.\n");
    }

    const char* title = cfg.is_ising ? (use_gpu ? "Ising Model -- GPU" : "Ising Model -- CPU")
                                     : (use_gpu ? "XY Model -- GPU" : "XY Model -- CPU");

    UIContext* ui = ui_init(title, cfg.width, cfg.height, cfg.font_path);
    if (!ui) {
        fprintf(stderr, "Failed to initialize UI Engine.\n");
        if (use_gpu) plugin_unload(&gpu);
        return 1;
    }

    void* host_grid = NULL;
    if (cfg.is_ising) {
        host_grid = malloc(cfg.width * cfg.height * sizeof(int));
        if (!host_grid) { fprintf(stderr, "Host memory allocation failed\n"); return 1; }
        ising_init_grid((int*)host_grid, cfg.width, cfg.height);
    } else {
        host_grid = malloc(cfg.width * cfg.height * sizeof(double));
        if (!host_grid) { fprintf(stderr, "Host memory allocation failed\n"); return 1; }
        xy_init_grid((double*)host_grid, cfg.width, cfg.height);
    }

    void* gpu_state = NULL;
    if (use_gpu) {
        gpu_state = gpu.init(cfg.height, cfg.width);
        gpu.upload_grid(gpu_state, host_grid);
        // Free host grid to save RAM in GPU mode
        free(host_grid);
        host_grid = NULL;
    }

    uint32_t* pixel_buffer = (uint32_t*)malloc(cfg.width * cfg.height * sizeof(uint32_t));
    if (!pixel_buffer) {
        fprintf(stderr, "Pixel buffer allocation failed.\n");
        return 1;
    }

    double T = cfg.default_T;
    double h = cfg.default_h;
    double J = cfg.J;

    double T_min = 0.0, T_max = 5.0, T_step = cfg.is_ising ? 0.1 : 0.05;
    double h_min = -5.0, h_max = 5.0, h_step = 0.1;

    while (ui_handle_events(ui, &T, &h, T_min, T_max, T_step, h_min, h_max, h_step)) {
        if (use_gpu) {
            gpu.update_grid(gpu_state, T, h, J);
            gpu.render_pixels(gpu_state, pixel_buffer);
        } else {
            if (cfg.is_ising) {
                ising_update_grid((int*)host_grid, cfg.width, cfg.height, T, h, J);
                ising_render_pixels((int*)host_grid, pixel_buffer, cfg.width, cfg.height);
            } else {
                xy_update_grid((double*)host_grid, cfg.width, cfg.height, T, h, J);
                xy_render_pixels((double*)host_grid, pixel_buffer, cfg.width, cfg.height);
            }
        }
        ui_render_frame(ui, pixel_buffer, T, h, J);
    }

    // Free buffers and cleanup
    free(pixel_buffer);
    if (use_gpu) {
        gpu.destroy(gpu_state);
        plugin_unload(&gpu);
    } else {
        free(host_grid);
    }
    ui_destroy(ui);

    return 0;
}
