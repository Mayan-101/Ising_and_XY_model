#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    void* (*init)(int height, int width);
    void (*destroy)(void* s);
    void (*upload_grid)(void* s, const void* host_grid);
    void (*update_grid)(void* s, double T, double h, double J);
    void (*render_pixels)(void* s, uint32_t* host_pixels);
    double (*measure_M)(void* s);
    void (*run_hysteresis)(void* s, double T, double h_max, int h_steps, int eq, int meas, double J);
    void (*save_csv)(void* s, const char* filename);
    bool loaded;
    void* handle; // DLL Handle
} SimulationPlugin;

bool plugin_load(SimulationPlugin *p, const char *dll_path);
void plugin_unload(SimulationPlugin *p);

#endif
