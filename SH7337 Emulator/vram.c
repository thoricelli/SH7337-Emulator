#include <string.h>
#include <SDL2/SDL.h>
#include "stdio.h"

#include "sdl.h"

#define PIXEL_BIT_SIZE 1
#define PIXEL_SIZE (PIXEL_BIT_SIZE * 8)

#define VRAM_SIZE (WINDOW_HEIGHT * WINDOW_WIDTH) / PIXEL_SIZE
#define WHITE 230

#define SCREEN_WIDTH_BYTES (WINDOW_WIDTH/PIXEL_SIZE)

unsigned char VRAM[VRAM_SIZE];

SDL_Renderer* renderer;

void set_sdl_renderer(SDL_Renderer* ptr) {
	renderer = ptr;
}

/*
* We need the, individual character size,
* the amount of characters and the amount of glyphs per column.
* 
* We then convert this to a black or white bit-array.
*/

//The width of 1 glyph.
#define GLYPH_WIDTH 5
//The height of 1 glyph.
#define GLYPH_HEIGHT 7

//The padding added to the glyph.
#define GLYPH_PADDING 1

//How many columns of glyphs there are.
#define ROWS 9
//How many glyphs there are per column.
#define GLYPHS_PER_ROW 16

#define TOTAL_GLYPHS ROWS * GLYPHS_PER_ROW

//Calculations
#define CEIL_DIV(x, y) (((x) + (y) - 1) / (y))

//This is the total glyph width (with the padding taken into account)
#define TOTAL_GLYPH_WIDTH (GLYPH_WIDTH + (GLYPH_PADDING * 2))
//This is the total glyph height (with the padding taken into account)
#define TOTAL_GLYPH_HEIGHT (GLYPH_HEIGHT + (GLYPH_PADDING * 2))

typedef struct {
    unsigned char glyph[CEIL_DIV((GLYPH_WIDTH * GLYPH_HEIGHT), PIXEL_SIZE)];
} font_character_t;

font_character_t font_data[ROWS * GLYPHS_PER_ROW];

unsigned int map_bit_get_32(unsigned int index, unsigned char* start) {
    unsigned int byte_index = index / PIXEL_SIZE;
    unsigned int bit_index = 7 - (index % PIXEL_SIZE);

    return start[byte_index] >> bit_index & 1;
}

void map_bit_set_32(unsigned int index, unsigned char value, unsigned char* start) {
    unsigned int byte_index = index / PIXEL_SIZE;
    unsigned int bit_index = 7 - (index % PIXEL_SIZE);
    unsigned int mask = 1 << bit_index;

    if (value)
        start[byte_index] |= mask;
    else
        start[byte_index] &= ~mask;
}

inline void debug_glyph(unsigned char glyph) {
    for (unsigned int i = 0; i < GLYPH_WIDTH * GLYPH_HEIGHT; i++)
    {
        if (i % GLYPH_WIDTH == 0)
            printf("\n");

        unsigned int byte_index = i / PIXEL_SIZE;
        unsigned int bit_index = 7 - (i % PIXEL_SIZE);

        unsigned char value = (font_data[glyph].glyph[byte_index] >> bit_index) & 1;

        if (value)
            printf("#");
        else
            printf(".");
    }
}

inline void fetch_glyph(unsigned int glyph_index, SDL_Surface* surface) {
    
    unsigned char r, g, b;

    //We will start at the GLYPH_PADDING, since the first pixel is padding.
    for (unsigned int glyph_y = 0; glyph_y < GLYPH_HEIGHT; glyph_y++)
    {
        for (unsigned int glyph_x = 0; glyph_x < GLYPH_WIDTH; glyph_x++)
        {
            unsigned int pixel_x = GLYPH_PADDING + ((glyph_index % GLYPHS_PER_ROW) * TOTAL_GLYPH_WIDTH) + glyph_x;
            unsigned int pixel_y = GLYPH_PADDING + ((glyph_index / GLYPHS_PER_ROW) * TOTAL_GLYPH_HEIGHT) + glyph_y;

            unsigned char* pixel_ptr = (unsigned char*)(surface->pixels) + (pixel_y * surface->pitch) + pixel_x * surface->format->BytesPerPixel;

            SDL_GetRGB(*((unsigned int*)pixel_ptr), surface->format, &r, &g, &b);

            unsigned int pixel_index = glyph_x + (glyph_y * GLYPH_WIDTH);

            map_bit_set_32(pixel_index, r == 0 && g == 0 && b == 0, font_data[glyph_index].glyph);
        }
    }

}

void generate_font_data(SDL_Surface* surface) {
    /*
    * We want to turn an image into 1 BPP font data.
    * Best way to do this is probably like this:
    * 
    * .XXX.     Here we scan per GLYPH_WIDTH, then move to the next row, so that the data looks like this:
    * X...X     .XXX. X..X X...X etc...
    * X...X     Then we can use this to draw to VRAM.
    * XXXXX
    * X...X
    * X...X
    * X...X
    */

    SDL_LockSurface(surface);

    for (unsigned int glyph_index = 0; glyph_index < ROWS * GLYPHS_PER_ROW; glyph_index++)
    {
        fetch_glyph(glyph_index, surface);
    }

    SDL_UnlockSurface(surface);
}

void draw_image(unsigned char image_x, unsigned char image_y, const unsigned char* data, unsigned char width, unsigned char height)
{
    for (unsigned int y = 0; y < height; y++)
    {
        for (unsigned int x = 0; x < width; x++)
        {
            unsigned int index_vram = ((image_y + y) * WINDOW_WIDTH + (x + image_x));

            unsigned int byte_index_vram = index_vram / 8;
            unsigned int bit_index_vram = index_vram % 8;

            unsigned int index_image = (y * width + x);

            unsigned char value = map_bit_get_32(index_image, data);

            unsigned char mask = 1 << (7 - bit_index_vram);

            if (value == 1)
                VRAM[byte_index_vram] |= mask;
            else
                VRAM[byte_index_vram] &= ~mask;
        }
    }
}

void draw_glyph(unsigned char glyph, unsigned char x, unsigned char y) {
    draw_image(x, y, font_data[glyph].glyph, GLYPH_WIDTH, GLYPH_HEIGHT);
}

void draw_vram() {
	for (unsigned int y = 0; y < WINDOW_HEIGHT; ++y)
	{
		for (unsigned int x = 0; x < WINDOW_WIDTH; ++x)
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

void clear_sdl_vram() {
    SDL_SetRenderDrawColor(renderer, WHITE, WHITE, WHITE, 255);
    SDL_RenderClear(renderer);
}

void print_vram(char* string, unsigned char x, unsigned char y) {

    x -= 1;
    y -= 1;

    do {
        draw_glyph(*string, (x * (GLYPH_WIDTH + GLYPH_PADDING)), (y * (GLYPH_HEIGHT + GLYPH_PADDING)));

        x++;
        string++;

    } while (*string != '\0');

    draw_vram();
}