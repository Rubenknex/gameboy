#include <iostream>
#include <iomanip>
#include <bitset>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <vector>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "SDL_FontCache.h"

#include "def.h"
#include "debug.h"
#include "gameboy.h"


const int SCREEN_W = PIXELS_W * 4;
const int SCREEN_H = PIXELS_H * 4;

const int FPS = 59.7;
const float MS_PER_FRAME = 1000.0 / FPS;

int main(int argc, char* args[])
{
    srand(time(NULL));

    //GameBoy gb("C:\\Users\\ruben\\Documents\\GitHub\\gameboy\\roms\\02-interrupts.gb");
    GameBoy gb("C:\\Users\\ruben\\Documents\\GitHub\\gameboy\\roms\\Tetris (World) (Rev A).gb");
    //GameBoy gb("roms/Dr. Mario (World).gb");
    //GameBoy gb("C:/Users/Ruben/Documents/ROMs/GameBoy/cpu_instrs/cpu_instrs.gb");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "Init error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "GameBoy",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_W+400,
        SCREEN_H,
        SDL_WINDOW_SHOWN);

    if (window == NULL) {
        std::cout << "Window creation error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        std::cout << "Renderer creation error: " << SDL_GetError() << std::endl;
        return -1;
    }

    std::cout << "main: " << renderer << std::endl;
    //SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    Debug debug = Debug(&gb, renderer);

    bool quit = false;
    SDL_Event e;
    int current_time = 0;
    bool redraw = false;

    bool stepping_mode = false;

    while (!quit) {
        current_time = SDL_GetTicks();

        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
            case SDL_QUIT:
                quit = true;
                break;
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        gb.buttons[Button::Up]     = keys[SDL_SCANCODE_W];
        gb.buttons[Button::Down]   = keys[SDL_SCANCODE_S];
        gb.buttons[Button::Left]   = keys[SDL_SCANCODE_A];
        gb.buttons[Button::Right]  = keys[SDL_SCANCODE_D];
        gb.buttons[Button::Start]  = keys[SDL_SCANCODE_RETURN];
        gb.buttons[Button::Select] = keys[SDL_SCANCODE_SPACE];
        gb.buttons[Button::A]      = keys[SDL_SCANCODE_O];
        gb.buttons[Button::B]      = keys[SDL_SCANCODE_P];


        if (keys[SDL_SCANCODE_Z])
            stepping_mode = true;
        if (keys[SDL_SCANCODE_X])
            stepping_mode = false;
        if (keys[SDL_SCANCODE_RIGHT])
            std::cout <<">" << std::endl;

        redraw = false;

        // Cycle the gameboy until it wants us to redraw the screen
        while (!redraw) {
            if (stepping_mode) {
                if (keys[SDL_SCANCODE_RIGHT]) {
                    gb.cpu.debug_print();
                    gb.cycle();
                    break;
                }
            } else {
                gb.cycle();
            }
            
            redraw = gb.gpu.get_redraw();
        }

        // Create an SDL_Texture from the gameboy screen buffer
        // This is not scaled up yet
        SDL_Surface* screen = SDL_CreateRGBSurfaceFrom(
            gb.gpu.get_screen_buffer(),
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

        // Clear the screen
        SDL_RenderClear(renderer);

        // Location for the gameboy screen
        SDL_Rect dst = {0, 0, SCREEN_W, SCREEN_H};

        // Render the gameboy texture onto the screen
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);

        // Debug info rendering
        debug.draw(SCREEN_W, 0);

        SDL_RenderPresent(renderer);

        // Regulate to 60 fps
        int elapsed_time = SDL_GetTicks() - current_time;
        if (elapsed_time < MS_PER_FRAME)
            SDL_Delay(MS_PER_FRAME - elapsed_time);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
    SDL_Quit();

    return 0;
}