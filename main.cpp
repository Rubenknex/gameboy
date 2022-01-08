#include <iostream>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "SDL_FontCache.h"
#include "fmt/format.h"

#include "def.h"
#include "debug.h"
#include "gameboy.h"


const int SCREEN_W = PIXELS_W * 4;
const int SCREEN_H = PIXELS_H * 4;

const int FPS = 59.7;
const float MS_PER_FRAME = 1000.0 / FPS;

int main(int argc, char* args[])
{
    //GameBoy gb("C:\\Users\\ruben\\Documents\\GitHub\\gameboy\\roms\\cpu_instrs.gb");
    //GameBoy gb("C:\\Users\\ruben\\Documents\\GitHub\\gameboy\\roms\\01-special.gb");
    //GameBoy gb("C:\\Users\\ruben\\Documents\\GitHub\\gameboy\\roms\\Tetris (World) (Rev A).gb");
    GameBoy gb("C:\\Users\\ruben\\Documents\\GitHub\\gameboy\\roms\\Super Mario Land (World).gb");
    //GameBoy gb("C:\\Users\\ruben\\Documents\\GitHub\\gameboy\\roms\\Dr. Mario (World).gb");
    //GameBoy gb("roms/Dr. Mario (World).gb");
    //GameBoy gb("C:/Users/Ruben/Documents/ROMs/GameBoy/cpu_instrs/cpu_instrs.gb");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "Init error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Init(SDL_INIT_AUDIO);

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

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 48000;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = 256;
    want.callback = NULL;
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0)
        std::cout << "Could not open audio device: " << SDL_GetError() << std::endl;
    SDL_PauseAudioDevice(dev, 0);

    //SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    Debug debug = Debug(&gb, renderer);

    bool quit = false;
    SDL_Event e;
    int current_time = 0;
    bool redraw = false;

    bool stepping_mode = false;
    u8 break_instr = 0;
    u16 break_PC = 0;

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
                default:
                    break;
                }
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

        // Wait until the audio queue is finished playing
        int queued = SDL_GetQueuedAudioSize(dev);
        while (queued > 0) {
            queued = SDL_GetQueuedAudioSize(dev);
        }

        bool sent_for_playback = false;

        // Cycle the gameboy until it wants us to redraw the screen
        while (!redraw && !stepping_mode) {
            gb.cycle();

            // The sample_queue_index can be 0 for multiple CPU cycles, so only
            // send the audio for playback once
            if (gb.apu.sample_queue_index == 0 && !sent_for_playback) {
                SDL_QueueAudio(dev, &gb.apu.sample_queue[0], gb.apu.sample_queue.size()*4);
                sent_for_playback = true;
            }

            if (gb.apu.sample_queue_index == 1) {
                // A new queue has started to fill
                sent_for_playback = false;
            }

            if (break_instr != 0 && gb.cpu.current_opcode() == break_instr)
                stepping_mode = true;
            if (break_PC != 0 && gb.cpu.PC == break_PC)
                stepping_mode = true;
            
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

        // Update the screen
        SDL_RenderPresent(renderer);

        // Regulate to 60 fps
        int elapsed_time = SDL_GetTicks() - current_time;
        if (elapsed_time < MS_PER_FRAME) {
            debug.current_fps = 1000.0 / elapsed_time;
            SDL_Delay(MS_PER_FRAME - elapsed_time);
        }
    }

    SDL_CloseAudioDevice(dev);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
    SDL_Quit();

    return 0;
}