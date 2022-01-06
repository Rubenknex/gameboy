#include <iostream>
#include <iomanip>
#include <bitset>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <numeric>

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

    //GameBoy gb("C:\\Users\\ruben\\Documents\\GitHub\\gameboy\\roms\\cpu_instrs.gb");
    //GameBoy gb("C:\\Users\\ruben\\Documents\\GitHub\\gameboy\\roms\\02-interrupts.gb");
    GameBoy gb("C:\\Users\\ruben\\Documents\\GitHub\\gameboy\\roms\\Tetris (World) (Rev A).gb");
    //GameBoy gb("roms/Dr. Mario (World).gb");
    //GameBoy gb("C:/Users/Ruben/Documents/ROMs/GameBoy/cpu_instrs/cpu_instrs.gb");
    gb.debug_mode = false;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "Init error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "GameBoy",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_W+700,
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

    //SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    Debug debug = Debug(&gb, renderer);

    bool quit = false;
    SDL_Event e;
    int current_time = 0;
    bool redraw = false;

    bool stepping_mode = false;

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    while (!quit) {
        current_time = SDL_GetTicks();

        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
            case SDL_KEYDOWN:
                switch (e.key.keysym.scancode) {
                case SDL_SCANCODE_Q:
                    stepping_mode = !stepping_mode;
                    break;
                case SDL_SCANCODE_RIGHT:
                    gb.cpu.debug_print();
                    gb.cycle();
                    break;
                case SDL_SCANCODE_LEFT:
                    stepping_mode = false;
                    break;
                default:
                    break;
                }
                break;
            case SDL_WINDOWEVENT:
                // Added for >1 windows
                if (e.window.event == SDL_WINDOWEVENT_CLOSE)
                    quit = true;
                break;
            case SDL_QUIT:
                quit = true;
                break;
            }
        }

        gb.buttons[Button::Up]     = keys[SDL_SCANCODE_W];
        gb.buttons[Button::Down]   = keys[SDL_SCANCODE_S];
        gb.buttons[Button::Left]   = keys[SDL_SCANCODE_A];
        gb.buttons[Button::Right]  = keys[SDL_SCANCODE_D];
        gb.buttons[Button::Start]  = keys[SDL_SCANCODE_RETURN];
        gb.buttons[Button::Select] = keys[SDL_SCANCODE_SPACE];
        gb.buttons[Button::A]      = keys[SDL_SCANCODE_O];
        gb.buttons[Button::B]      = keys[SDL_SCANCODE_P];

        redraw = false;

        // Cycle the gameboy until it wants us to redraw the screen
        while (!redraw && !stepping_mode) {
            gb.cycle();

            //if (gb.cpu.current_opcode() == 0x76)
            //    stepping_mode = true;
            
            redraw = gb.gpu.get_redraw();
        }

        // Clear the screen
        SDL_RenderClear(renderer);

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

        // Render the gameboy texture onto the screen
        SDL_Rect dst = {0, 0, SCREEN_W, SCREEN_H};
        SDL_RenderCopy(renderer, texture, NULL, &dst);

        //SDL_Rect dst2 = {SCREEN_W + 20, 150, 16*8, 24*8};
        SDL_DestroyTexture(texture);

        // Debug info rendering
        debug.draw(SCREEN_W, 0);


        // Tileset data
        unsigned int palette[4] = {0xFFFFFFFF,0xAAAAAAFF,0x555555FF,0x000000FF};
        std::vector<int> conv;
        conv.resize(16*24*8*8);

        // Convert the gameboy tiledata of a series of 64 values to an array
        // that we can draw on the screen
        for (int tile_x = 0; tile_x < 16; tile_x++) {
            for (int tile_y = 0; tile_y < 24; tile_y++) {
                for (int pixel = 0; pixel < 8*8; pixel++) {
                    int tile_number = tile_x * 16 + tile_y;
                    int screen_pixel = tile_y * 16 * 64 + (int)(pixel/8) * 16 * 8 + tile_x * 8 + pixel % 8;

                    conv[screen_pixel] = palette[gb.gpu.tileset[tile_number*64 + pixel]];
                }
            }
        }

        // 16x24 tiles of 8x8 pixels
        screen = SDL_CreateRGBSurfaceFrom(
            &conv[0],
            16*8, // width
            24*8, // height
            32, // bits per pixel
            16*8*4, // bytes per row of pixels
            0xFF000000,
            0x00FF0000,
            0x0000FF00,
            0x000000FF);

        texture = SDL_CreateTextureFromSurface(renderer, screen);
        SDL_FreeSurface(screen);

        // Render the gameboy texture onto the screen
        dst = {SCREEN_W + 400, 20, 16*8*2, 24*8*2};
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);

        // Update the screen
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