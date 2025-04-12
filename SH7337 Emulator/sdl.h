#pragma once

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 128
#define WINDOW_HEIGHT 64

#define SCALE 4

void sdl_log_error();
void sdl_create(SDL_Window* window, SDL_Renderer* renderer);
void sdl_destroy(const SDL_Window* window, const SDL_Renderer* renderer);