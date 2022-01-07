#include "debug.h"

#include <iostream>
#include <bitset>
#include <iomanip>
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
}