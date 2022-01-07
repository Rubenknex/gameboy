#include "debug.h"

#include <SDL2/SDL.h>
#include <iostream>
#define FMT_HEADER_ONLY
#include "fmt/format.h"

#include "gameboy.h"

Debug::Debug(GameBoy* gb, SDL_Renderer* renderer) : m_gb(gb) , m_renderer(renderer) {
    m_font = FC_CreateFont();
    FC_LoadFont(m_font, m_renderer, "monogram.ttf", 32, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);
}

Debug::~Debug() {
    FC_FreeFont(m_font);
}

void Debug::draw(int x, int y) {
    // CPU
    FC_Draw(m_font, m_renderer, x + 20, 20, fmt::format("PC={0:04X} SP={1:04X}", m_gb->cpu.PC, m_gb->cpu.SP).c_str());
    FC_Draw(m_font, m_renderer, x + 20, 40, fmt::format("AF={0:04X} BC={1:04X}", m_gb->cpu.AF.get(), m_gb->cpu.BC.get()).c_str());
    FC_Draw(m_font, m_renderer, x + 20, 60, fmt::format("DE={0:04X} HL={1:04X}", m_gb->cpu.DE.get(), m_gb->cpu.HL.get()).c_str());

    // Interrupts
    FC_Draw(m_font, m_renderer, x + 20, 80, fmt::format("IME={0:05b} halted={1:1b}", m_gb->cpu.interrupt_master_enable, m_gb->cpu.halted).c_str());
    FC_Draw(m_font, m_renderer, x + 20, 100, fmt::format("IF={0:05b} IE={1:05b} IME={2:b}", m_gb->mmu.interrupt_flags, m_gb->mmu.interrupt_enable, m_gb->cpu.interrupt_master_enable).c_str());

    // Timer
    FC_Draw(m_font, m_renderer, x + 20, 140, fmt::format("DIV={0:2X} TIMA={1:2X} TMA={2:2X} TAC={3:2X}", m_gb->mmu.divide_register, m_gb->mmu.timer_counter, m_gb->mmu.timer_modulo, m_gb->mmu.timer_control).c_str());


    // VRAM
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

                conv[screen_pixel] = palette[m_gb->gpu.tileset[tile_number*64 + pixel]];
            }
        }
    }

    // 16x24 tiles of 8x8 pixels
    SDL_Surface* screen = SDL_CreateRGBSurfaceFrom(
        &conv[0],
        16*8, // width
        24*8, // height
        32, // bits per pixel
        16*8*4, // bytes per row of pixels
        0xFF000000,
        0x00FF0000,
        0x0000FF00,
        0x000000FF);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, screen);
    SDL_FreeSurface(screen);

    // Render the gameboy texture onto the screen
    SDL_Rect dst = {x + 400, 20, 16*8*2, 24*8*2};
    SDL_RenderCopy(m_renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
}