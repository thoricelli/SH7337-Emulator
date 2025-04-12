#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>

#include "cpu.h"
#include "mmu.h"
#include "gdb.h"

#define WINDOW_WIDTH 128
#define WINDOW_HEIGHT 64

#define SCALE 4

#define GDB_ENABLED 1

void sdl_log_error() {
    printf("SDL Error. %s\n", SDL_GetError());
}

inline void sdl_create (SDL_Window* window, SDL_Renderer* renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        sdl_log_error();
        return 1;
    }

    window = SDL_CreateWindow(
        "SH7337 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH * SCALE,
        WINDOW_HEIGHT * SCALE,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        sdl_log_error();
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        sdl_log_error();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
}

inline void sdl_destroy(const SDL_Window* window, const SDL_Renderer* renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

#define HEADER_SIZE 0x200

void *load_file(const char* filename) {
    FILE* handle = fopen(filename, "rb");

    struct stat st;

    stat(filename, &st);

    void* memory = malloc(st.st_size - HEADER_SIZE);

    fseek(handle, 0x200, SEEK_SET);
    fread(memory, 1, st.st_size - HEADER_SIZE, handle);

    fclose(handle);

    return memory;
}

int main(void)
{
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    sdl_create(window, renderer);

    void* rom = load_file("TEST.G1A");

    int running = 1;
    SDL_Event event;

    cpu_running_state_t* state = get_cpu_running_state();

    state->paused = 0;
    state->step = 0;

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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    sdl_destroy(window, renderer);
    return 0;
}