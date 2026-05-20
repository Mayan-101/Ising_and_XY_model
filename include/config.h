#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef struct {
    int width;
    int height;
    double default_T;
    double default_h;
    double J;
    const char* font_path;
    bool is_ising;
    bool use_gpu;
    
    // Hysteresis parameters
    double h_max;
    int h_steps;
    int equil_sweeps;
    int meas_sweeps;
} SimConfig;

void init_config(SimConfig* cfg, int argc, char** argv);

#endif
