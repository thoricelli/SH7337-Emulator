#pragma once

#include <SDL2/SDL.h>

#define WINDOW_PADDING 1

#define WINDOW_WIDTH 128
#define WINDOW_HEIGHT 64

#define TOT_PADDING (WINDOW_PADDING * 2)

#define TOT_WINDOW_WIDTH (WINDOW_WIDTH + TOT_PADDING)
#define TOT_WINDOW_HEIGHT (WINDOW_HEIGHT + TOT_PADDING)

#define SCALE 4

void sdl_log_error();
void sdl_create(SDL_Window* window, SDL_Renderer* renderer);
void sdl_destroy(const SDL_Window* window, const SDL_Renderer* renderer);