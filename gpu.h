#ifndef GPU_H
#define GPU_H

#include <vector>

#include "def.h"

class GameBoy;

namespace GPUMode{
    //enum Type {OAM, VRAM, HBlank, VBlank};
    enum Type {HBlank, VBlank, OAM, VRAM};
}

struct Sprite {
    u8 y, x, tile;
    bool priority, y_flip, x_flip, palette;
};

class GPU {
public:
    GPU(GameBoy* gb);

    u8 read_byte(u16 address);
    void write_byte(u16 address, u8 value);

    void cycle();
    void reset();
    void update_tile(u16 address, u8 value);
    void update_object(u16 address, u8 value);
    void render_scanline();

    u8 get_lcd_byte();
    void set_lcd_byte(u8 lcd);
    bool get_redraw();
    int* get_screen_buffer();
    void dump_vram();

public:
    // FF40 LCD control byte
    bool lcd_enabled;
    bool window_tilemap; // 0: 9800-9BFF, 1: 9C00-9FFF
    bool window_enabled;
    bool background_tileset; // 0: 8800-97FF, 1: 8000-8FFF
    bool background_tilemap; // 0: 9800-9BFF, 1: 9C00-9FFF
    bool sprite_size; // 0: 8x8, 1: 8x16
    bool sprites_enabled;
    bool background_enabled;

    // FF41 STAT (LCD status)
    u8 lcd_status;

    // FF43 & FF42 scroll bytes
    u8 scroll_x, scroll_y;
    // FF44 scanline byte
    u8 current_line;
    u8 ly_compare;

    // RGB color values for white, light gray, dark gray and black
    int color_palette[4];
    // FF47: background & window palette data
    // gray shades corresponding to the color numbers 0-3
    u8 background_palette;
    // FF48
    u8 sprite_palette_0;
    // FF49
    u8 sprite_palette_1;

public:
    GameBoy* gb;

    GPUMode::Type mode;
    int cycles;
    bool redraw;
    std::vector<int> screen;
    std::vector<u8> tileset;
    std::vector<Sprite> sprites;
};

#endif