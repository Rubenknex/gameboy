#ifndef GAMEBOY_H
#define GAMEBOY_H

#include <string>

#include "cartridge.h"
#include "apu.h"
#include "cpu.h"
#include "gpu.h"
#include "mmu.h"

namespace Button {
	enum Type {Up, Down, Left, Right, Start, Select, A, B};
}

class GameBoy {
public:
    GameBoy(const std::string& filename);
    ~GameBoy();

    u8 read_byte(u16 address);
    void write_byte(u16 address, u8 value);

    void cycle();

public:
	bool buttons[8];
    Cartridge cartridge;
    APU apu;
    CPU cpu;
    GPU gpu;
    MMU mmu;

    // FF00 joypad input byte
    bool select_button;
    bool select_direction;
    bool down_or_start;
    bool up_or_select;
    bool left_or_b;
    bool right_or_a;

    u8 divide_register; // FF04, DIV
    float raw_timer_counter;
    u8 timer_counter; // FF05, TIMA
    u8 timer_modulo; // FF06, TMA
    u8 timer_control; // FF07, TAC

    u8 disable_bios; // FF50

    u8 interrupt_flags; // FF0F, IF
    u8 interrupt_enable; // FFFF, IE

    bool debug_mode;
};

#endif