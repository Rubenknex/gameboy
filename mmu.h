#ifndef MMU_H
#define MMU_H

#include <stdlib.h>
#include <vector>

#include "cartridge.h"
#include "def.h"

class Cartridge;
class GameBoy;

/*
    Memory map layout
    http://gameboy.mongenel.com/dmg/asmmemmap.html

    0000-00FF   Interrupt routines
                VBlank: 0x40
                LCDC:   0x48
                Timer:  0x50
                Serial: 0x58
                Joypad: 0x60

    0100-014F   Cartridge header
    0150-3FFF   ROM bank 0 (fixed)
    4000-7FFF   Switchable ROM banks

    VRAM
    8000-87FF   Tileset #1: tiles 0 to 127, 16 bytes per tile
    8800-8FFF   Tileset #1: tiles 128 to 255
                Tileset #0: tiles -1 to -128
    9000-97FF   Tileset #0: tiles 0 to 127
    9800-9BFF   Tilemap #0: 32x32=1024 tiles (bytes)
    9C00-9FFF   Tilemap #1: 32x32=1024 tiles (bytes)

    C000-CFFF   Internal RAM bank 0 (fixed)
    D000-DFFF   Switchable RAM banks

    FE00-FE9F   Object Attribute Memory

    FF00-FF7F   Hardware registers

    FF80-FFFE   High RAM

*/

class MMU {
public:
    MMU(GameBoy* gb, const Cartridge& cartridge);
    ~MMU();

    u8 read_byte(u16 address);
    void write_byte(u16 address, u8 value);

    u16 read_word(u16 address);
    void write_word(u16 address, u16 value);

public:
    GameBoy* gb;

    bool in_bios;
    MemoryMapper::Type mapper;
    std::vector<u8> rom_0, rom_1, vram, eram, wram, oam, hram;

    // FF00 joypad input byte
    bool select_button;
    bool select_direction;
    bool down_or_start;
    bool up_or_select;
    bool left_or_b;
    bool right_or_a;

    // FF04 divide register
    u8 divide_register;
    u8 timer_counter;
    u8 timer_modulo;
    u8 timer_control;

    // FF0F interrupt flags byte
    u8 interrupt_flags;
    // FFFF interrupt enable byte
    u8 interrupt_enable;
};

#endif