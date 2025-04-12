#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>

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

    void* rom = load_file("TEST.G1A");

    int running = 1;
    SDL_Event event;

    cpu_running_state_t* state = get_cpu_running_state();

    set_sdl_renderer(renderer);

    set_rom_ptr(rom);
    
    set_register(0x88020000, 15);
    set_pc_little_endian(0x00300200);

    if (GDB_ENABLED) {
        pause_cpu();

        SDL_Thread* thread = SDL_CreateThread(gdb_loop, "gdb", NULL);
    }

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = 0;
        }

        if (!state->paused)
            cpu_tick();

        draw_vram();

        SDL_RenderPresent(renderer);
    }

    free(rom);
    sdl_destroy(window, renderer);
    return 0;
}