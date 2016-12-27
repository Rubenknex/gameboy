#include <iostream>
#include <vector>

#include <SDL2/SDL.h>

#include "def.h"
#include "gameboy.h"

#define SCREEN_W 160
#define SCREEN_H 144

#define FPS 60.0
#define MS_PER_FRAME 1000.0 / FPS

int main(int argc, char* args[])
{
    GameBoy gb("roms/Tetris (World).gb");
    //GameBoy gb("roms/Dr. Mario (World).gb");
    //GameBoy gb("C:/Users/Ruben/Documents/ROMs/GameBoy/cpu_instrs/cpu_instrs.gb");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not be initialized! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("GameBoy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_W * 2, SCREEN_H * 2, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    bool quit = false;
    SDL_Event e;
    int current_time = 0;
    bool redraw = false;

    while (!quit) {
        current_time = SDL_GetTicks();

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT)
                quit = true;
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);

        redraw = false;

        // Cycle the gameboy until it wants us to redraw the screen
        while (!redraw) {
            gb.cycle();
            redraw = gb.gpu.get_redraw();
        }

        // Create an SDL_Texture from the gameboy screen buffer
        SDL_Surface* screen = SDL_CreateRGBSurfaceFrom(gb.gpu.get_screen_buffer(),
                                                       PIXELS_W,
                                                       PIXELS_H,
                                                       32,
                                                       PIXELS_W * 4,
                                                       0xFF000000,
                                                       0x00FF0000,
                                                       0x0000FF00,
                                                       0x000000FF);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, screen);
        SDL_FreeSurface(screen);

        // Render the screen
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Regulate to 60 fps
        int elapsed_time = SDL_GetTicks() - current_time;
        if (elapsed_time < MS_PER_FRAME)
            SDL_Delay(MS_PER_FRAME - elapsed_time);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << std::cin.get();

    return 0;
}