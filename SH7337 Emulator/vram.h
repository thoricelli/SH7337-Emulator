#pragma once

#include <SDL2/SDL.h>

void generate_font_data(SDL_Surface* surface);
void set_sdl_renderer(SDL_Renderer* ptr);

void draw_vram();
void clear_vram();
void clear_sdl_vram();

void print_vram(char* string, unsigned char x, unsigned char y);