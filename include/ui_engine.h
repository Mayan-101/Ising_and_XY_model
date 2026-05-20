#ifndef UI_ENGINE_H
#define UI_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct UIContext UIContext;

UIContext* ui_init(const char* title, int width, int height, const char* font_path);
void ui_destroy(UIContext* ctx);

// Returns true if running, updates T and h via pointers, and returns false when quitting
bool ui_handle_events(UIContext* ctx, double* T, double* h, double T_min, double T_max, double T_step, double h_min, double h_max, double h_step);

// Render the pixel array and HUD
void ui_render_frame(UIContext* ctx, uint32_t* pixels, double T, double h, double J);

#endif
