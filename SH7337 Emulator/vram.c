#include <string.h>
#include <SDL2/SDL.h>

#include "sdl.h"

#define VRAM_SIZE (WINDOW_HEIGHT * WINDOW_WIDTH) / 8
#define WHITE 230

unsigned char VRAM[VRAM_SIZE];

SDL_Renderer* renderer;

void set_sdl_renderer(SDL_Renderer* ptr) {
	renderer = ptr;
}

void draw_vram() {
	for (size_t y = 0; y < WINDOW_HEIGHT; ++y)
	{
		for (size_t x = 0; x < WINDOW_WIDTH; ++x)
		{
            int byte_index = (y * WINDOW_WIDTH + x) / 8;
            int bit_index = (y * WINDOW_WIDTH + x) % 8;
            unsigned char bit = (VRAM[byte_index] >> (7 - bit_index)) & 1;

            if (bit == 1) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }
            else {
                SDL_SetRenderDrawColor(renderer, WHITE, WHITE, WHITE, 255);
            }

            SDL_RenderDrawPoint(renderer, x, y);
		}
	}
}

void clear_vram() {
	memset(VRAM, 0, VRAM_SIZE);
}