#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "sdl.h"

void sdl_log_error() {
    printf("SDL Error. %s\n", SDL_GetError());
}

void sdl_create(SDL_Window** window, SDL_Renderer** renderer) {
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        sdl_log_error();
        return 1;
    }

    *window = SDL_CreateWindow(
        "SH7337 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        TOT_WINDOW_WIDTH * SCALE,
        WINDOW_HEIGHT * SCALE,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!*window) {
        sdl_log_error();
        SDL_Quit();
        return 1;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);

    SDL_RenderSetLogicalSize(*renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    if (!renderer) {
        sdl_log_error();
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return 1;
    }
}

void sdl_destroy(const SDL_Window* window, const SDL_Renderer* renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}