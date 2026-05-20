#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "ui_engine.h"
#include <stdio.h>
#include <stdlib.h>

struct UIContext {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_Texture* texture;
    int width;
    int height;
};

UIContext* ui_init(const char* title, int width, int height, const char* font_path) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return NULL;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
    }

    UIContext* ctx = (UIContext*)malloc(sizeof(UIContext));
    if (!ctx) return NULL;

    ctx->width = width;
    ctx->height = height;

    ctx->window = SDL_CreateWindow(title,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   width, height,
                                   SDL_WINDOW_SHOWN);
    if (!ctx->window) {
        SDL_Quit();
        free(ctx);
        return NULL;
    }

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED);
    if (!ctx->renderer) {
        SDL_DestroyWindow(ctx->window);
        SDL_Quit();
        free(ctx);
        return NULL;
    }

    ctx->font = TTF_OpenFont(font_path, 24);
    if (!ctx->font) {
        fprintf(stderr, "Font loading failed (\"%s\"): %s\n", font_path, TTF_GetError());
        // Attempt a fallback path just in case
        ctx->font = TTF_OpenFont("C:/Windows/Fonts/consola.ttf", 24);
        if (!ctx->font) {
            fprintf(stderr, "Fallback font loading failed.\n");
        }
    }

    ctx->texture = SDL_CreateTexture(ctx->renderer,
                                     SDL_PIXELFORMAT_RGBA32,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     width, height);
    if (!ctx->texture) {
        if (ctx->font) TTF_CloseFont(ctx->font);
        SDL_DestroyRenderer(ctx->renderer);
        SDL_DestroyWindow(ctx->window);
        TTF_Quit();
        SDL_Quit();
        free(ctx);
        return NULL;
    }

    return ctx;
}

void ui_destroy(UIContext* ctx) {
    if (!ctx) return;
    if (ctx->texture) SDL_DestroyTexture(ctx->texture);
    if (ctx->font) TTF_CloseFont(ctx->font);
    if (ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window) SDL_DestroyWindow(ctx->window);
    TTF_Quit();
    SDL_Quit();
    free(ctx);
}

bool ui_handle_events(UIContext* ctx, double* T, double* h, double T_min, double T_max, double T_step, double h_min, double h_max, double h_step) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        } else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_UP:
                    *T += T_step;
                    if (*T > T_max) *T = T_max;
                    break;
                case SDLK_DOWN:
                    *T -= T_step;
                    if (*T < T_min) *T = T_min;
                    break;
                case SDLK_RIGHT:
                    *h += h_step;
                    if (*h > h_max) *h = h_max;
                    break;
                case SDLK_LEFT:
                    *h -= h_step;
                    if (*h < h_min) *h = h_min;
                    break;
                case SDLK_ESCAPE:
                    return false;
            }
        }
    }
    return true;
}

static void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) return;

    int texW = 0, texH = 0;
    SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
    SDL_Rect dst = {x, y, texW, texH};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
}

void ui_render_frame(UIContext* ctx, uint32_t* pixels, double T, double h, double J) {
    SDL_UpdateTexture(ctx->texture, NULL, pixels, ctx->width * sizeof(uint32_t));
    SDL_RenderClear(ctx->renderer);
    SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);

    char temp_text[64];
    char field_text[64];
    char j_text[64];
    snprintf(temp_text, sizeof(temp_text), "T = %.2f  [Up/Down]", T);
    snprintf(field_text, sizeof(field_text), "B = %+.2f  [Left/Right]", h);
    snprintf(j_text, sizeof(j_text), "J = %.2f", J);
    
    // Aesthetic drop shadow effect for perfect readability on all simulation states
    SDL_Color shadow = {0, 0, 0, 255};
    SDL_Color text_color = {255, 255, 255, 255};
    
    // Draw shadow offsets
    render_text(ctx->renderer, ctx->font, temp_text, 11, 11, shadow);
    render_text(ctx->renderer, ctx->font, field_text, 11, 41, shadow);
    
    // Draw foreground text
    render_text(ctx->renderer, ctx->font, temp_text, 10, 10, text_color);
    render_text(ctx->renderer, ctx->font, field_text, 10, 40, text_color);

    render_text(ctx->renderer, ctx->font, j_text, 11, 71, shadow);
    render_text(ctx->renderer, ctx->font, j_text, 10, 70, text_color);

    SDL_RenderPresent(ctx->renderer);
    SDL_Delay(10);
}
