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
    MMU(GameBoy* gb);
    ~MMU();

    u8 read_byte(u16 address);
    void write_byte(u16 address, u8 value);

    u16 read_word(u16 address);
    void write_word(u16 address, u16 value);

public:
    GameBoy* gb;

    u8 current_rom_bank;

    std::vector<u8> vram, eram, wram, oam, hram;
};

#endif