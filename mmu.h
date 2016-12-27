#ifndef MMU_H
#define MMU_H

#include <vector>

#include "cartridge.h"
#include "def.h"

class Cartridge;
class GameBoy;

/*
    Memory map layout

    VRAM
    8000-87FF   Tileset #1: tiles 0 to 127, 16 bytes per tile
    8800-8FFF   Tileset #1: tiles 128 to 255
                Tileset #0: tiles -1 to -128
    9000-97FF   Tileset #0: tiles 0 to 127
    9800-9BFF   Tilemap #0: 32x32=1024 tiles (bytes)
    9C00-9FFF   Tilemap #1: 32x32=1024 tiles (bytes)

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
    std::vector<u8> rom_0, rom_1, vram, eram, wram, hram;

    // FF00 joypad input byte
    bool select_button;
    bool select_direction;
    bool down_or_start;
    bool up_or_select;
    bool left_or_b;
    bool right_or_a;

    // FF0F interrupt flags byte
    u8 interrupt_flags;
    // FFFF interrupt enable byte
    u8 interrupt_enable;
};

#endif