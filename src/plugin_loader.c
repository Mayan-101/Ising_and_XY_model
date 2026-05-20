#include "plugin_loader.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define BIND(p, T, name)                                                     \
    (p)->name = (T)GetProcAddress((HMODULE)(p)->handle, "gpu_" #name);       \
    if (!(p)->name) {                                                        \
        fprintf(stderr, "plugin_loader: missing symbol gpu_" #name "\n");    \
        FreeLibrary((HMODULE)(p)->handle); (p)->handle = NULL; return false; \
    }

bool plugin_load(SimulationPlugin *p, const char *dll_path)
{
    memset(p, 0, sizeof(*p));

    p->handle = LoadLibraryA(dll_path);
    if (!p->handle) {
        // Silent fail so we can try fallback paths or CPU
        return false;
    }

    BIND(p, void* (*)(int, int),                      init)
    BIND(p, void (*)(void*),                           destroy)
    BIND(p, void (*)(void*, const void*),              upload_grid)
    BIND(p, void (*)(void*, double, double, double),           update_grid)
    BIND(p, void (*)(void*, uint32_t*),                render_pixels)
    BIND(p, double (*)(void*),                         measure_M)
    BIND(p, void (*)(void*, double, double, int, int, int, double), run_hysteresis)
    BIND(p, void (*)(void*, const char*),              save_csv)

    p->loaded = true;
    printf("plugin_loader: loaded \"%s\"\n", dll_path);
    return true;
}

void plugin_unload(SimulationPlugin *p)
{
    if (p && p->handle) {
        FreeLibrary((HMODULE)p->handle);
        p->handle = NULL;
        p->loaded = false;
    }
}
