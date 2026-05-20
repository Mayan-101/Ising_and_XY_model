#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_config(SimConfig* cfg, int argc, char** argv) {
    // Default configuration
    cfg->width = 800;
    cfg->height = 800;
    cfg->is_ising = true;
    cfg->use_gpu = true;
    cfg->font_path = "assets/font.ttf";
    cfg->J = 1.0;
    
    // Dynamically detect model from executable name
    if (argv && argv[0]) {
        if (strstr(argv[0], "xy") != NULL || strstr(argv[0], "XY") != NULL) {
            cfg->is_ising = false;
        }
    }

    // Model specific defaults
    if (cfg->is_ising) {
        cfg->default_T = 1.5;
        cfg->h_max = 3.0;
    } else {
        cfg->default_T = 0.89;
        cfg->h_max = 3.0;
    }
    cfg->default_h = 0.0;
    
    // Sweep defaults
    cfg->h_steps = 300;
    cfg->equil_sweeps = 500;
    cfg->meas_sweeps = 200;

    int pos_arg_count = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--model") == 0 && i + 1 < argc) {
            if (strcmp(argv[i+1], "xy") == 0) {
                cfg->is_ising = false;
                cfg->default_T = 0.89;
            } else {
                cfg->is_ising = true;
                cfg->default_T = 1.5;
            }
            i++;
        } else if (strcmp(argv[i], "--cpu") == 0) {
            cfg->use_gpu = false;
        } else if (strcmp(argv[i], "--gpu") == 0) {
            cfg->use_gpu = true;
        } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            cfg->width = atoi(argv[i+1]);
            i++;
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            cfg->height = atoi(argv[i+1]);
            i++;
        } else if (strcmp(argv[i], "--font") == 0 && i + 1 < argc) {
            cfg->font_path = argv[i+1];
            i++;
        } else if (strcmp(argv[i], "--J") == 0 && i + 1 < argc) {
            cfg->J = atof(argv[i+1]);
            i++;
        } else if (argv[i][0] != '-') {
            // Positional arguments: [T] [h_max] [h_steps] [equil] [meas]
            pos_arg_count++;
            if (pos_arg_count == 1) {
                cfg->default_T = atof(argv[i]);
            } else if (pos_arg_count == 2) {
                cfg->h_max = atof(argv[i]);
            } else if (pos_arg_count == 3) {
                cfg->h_steps = atoi(argv[i]);
            } else if (pos_arg_count == 4) {
                cfg->equil_sweeps = atoi(argv[i]);
            } else if (pos_arg_count == 5) {
                cfg->meas_sweeps = atoi(argv[i]);
            } else if (pos_arg_count == 6) {
                cfg->J = atof(argv[i]);
            }
        }
    }
}
