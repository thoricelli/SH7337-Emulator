#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "cpu.h"
#include "mmu.h"
#include "gdb.h"
#include "file.h"
#include "sdl.h"
#include "syscall.h"
#include "vram.h"

#define GDB_ENABLED 1

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

int main(void)
{
    sdl_create(&window, &renderer);

    SDL_Surface* image = IMG_Load("font.png");

    generate_font_data(image);
    set_sdl_renderer(renderer);


    void* rom = load_file("TEST.G1A");

    int running = 1;
    SDL_Event event;

    cpu_running_state_t* state = get_cpu_running_state();

    set_rom_ptr(rom);
    
    set_register(0x88020000, 15);
    set_pc_little_endian(0x00300200);

    if (GDB_ENABLED) {
        pause_cpu();

        SDL_Thread* thread = SDL_CreateThread(gdb_loop, "gdb", NULL);
    }

    clear_sdl_vram();
    clear_vram();

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                case SDL_WINDOWEVENT:
                    clear_sdl_vram();
                    draw_vram();
                    break;
            }
        }

        if (!state->paused)
            cpu_tick();

        SDL_RenderPresent(renderer);
    }

    free(rom);
    sdl_destroy(window, renderer);
    return 0;
}