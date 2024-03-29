#include "gpu.h"

#include <iostream>

#include "gameboy.h"

GPU::GPU(GameBoy* gb) : gb(gb), mode(GPUMode::HBlank), cycles(0) {
    lcd_enabled = false;
    window_tilemap = false;
    window_enabled = false;
    background_tileset = false;
    background_tilemap = false;
    sprite_size = false;
    sprites_enabled = false;
    background_enabled = false;

    lcd_status = 0;

    scroll_x = scroll_y = 0;
    current_line = 0;
    ly_compare = 0;

    background_palette = 0;

    color_palette[0] = 0xFFFFFFFF;
    color_palette[1] = 0xAAAAAAFF;
    color_palette[2] = 0x555555FF;
    color_palette[3] = 0x000000FF;

    color_palette[0] = 0xaea691FF;
    color_palette[1] = 0x887b6aFF;
    color_palette[2] = 0x605444FF;
    color_palette[3] = 0x4e3f2aFF;

    color_palette[0] = 0xe0f8d0FF;
    color_palette[1] = 0x88c070FF;
    color_palette[2] = 0x346856FF;
    color_palette[3] = 0x081820FF;

    redraw = false;

    screen.resize(PIXELS_W * PIXELS_H);
    std::fill(screen.begin(), screen.end(), color_palette[0]);

    // 256+128=384 unique tiles, each consisting of 8*8 pixels
    tileset.resize((256 + 128) * 8 * 8);

    // Initialize the sprites collection
    for (int i = 0; i < 40; i++) {
        Sprite s = {0, 0, 0, false, false, false, false};
        sprites.push_back(s);
    }
}

u8 GPU::read_byte(u16 address) {
    u8 result = 0;

    switch (address & 0xFF) {
        case 0x40: // LCD Control
            // Construct the LCD control byte from the gpu parameters
            result = get_lcd_byte();
            break;
        case 0x41: // LCD Status
            lcd_status = (lcd_status & ~0b11) | mode;
            result = lcd_status;
            break;
        case 0x42: // Scroll Y
            result = scroll_y;
            break;
        case 0x43: // Scroll X
            result = scroll_x;
            break;
        case 0x44: // Y Coord
            result = current_line;
            break;
        case 0x45: // LY compare
            result = ly_compare;
            break;
        case 0x46: // DMA transfer start address

            break;
        case 0x47: // BG and window palette data
            result = background_palette;
            break;
        case 0x48: // Object palette 0 data
            result = sprite_palette_0;
            break;
        case 0x49: // Object palette 1 data
            result = sprite_palette_1;
            break;
        case 0x4A: // Window Y

            break;
        case 0x4B: // Window X

            break;
        default: {
            result = 0;
        }
    }

    return result;
}

void GPU::write_byte(u16 address, u8 value) {
    switch (address & 0xFF) {
    case 0x40: // LCD Control
        set_lcd_byte(value);
        break;
    case 0x41: { // LCD Status
        // Set only bits 3-6
        u8 mask = 0b01111000;
        lcd_status = (lcd_status & ~mask) | (value & mask);
        } break;
    case 0x42: // Scroll Y
        scroll_y = value;
        break;
    case 0x43: // Scroll X
        scroll_x = value;
        break;
    case 0x45: // LY compare
        ly_compare = value;
        break;
    case 0x46: { // DMA transfer start address
        u16 start_addr = (value << 8);

        for (int i = 0; i < 0x9F; i++)
            gb->mmu.write_byte(0xFE00 + i, gb->mmu.read_byte(start_addr + i));

        } break;
    case 0x47: // BG and window palette data
        background_palette = value;
        break;
    case 0x48: // Object palette 0 data
        sprite_palette_0 = value;
        break;
    case 0x49: // Object palette 1 data
        sprite_palette_1 = value;
        break;
    case 0x4A: // Window Y

        break;
    case 0x4B: // Window X

        break;
    }
}

void GPU::cycle() {
    redraw = false;

    cycles += gb->cpu.get_elapsed_cycles();

    switch (mode) {
    case GPUMode::OAM: // Draw sprites
        if (cycles >= T_LINE_OAM) {
            cycles -= T_LINE_OAM;

            // Finished drawing sprites, move to drawing tiles
            mode = GPUMode::VRAM;
        }
        break;
    case GPUMode::VRAM: // Draw background tiles
        if (cycles >= T_LINE_VRAM) {
            cycles -= T_LINE_VRAM;

            // Finished drawing tiles, move to HBlank
            mode = GPUMode::HBlank;
            if (GET_BIT(lcd_status, 3))
                    gb->interrupt_flags |= INTERRUPT_LCDC;

            if (lcd_enabled)
                render_scanline();
        }
        break;
    case GPUMode::HBlank:
        if (cycles >= T_HBLANK) {
            cycles -= T_HBLANK;
            current_line++;

            // If we finish the HBlank of the last line, set the flag
            // to redraw the screen, and move to VBlank
            if (current_line == PIXELS_H - 1) {
                mode = GPUMode::VBlank;
                if (GET_BIT(lcd_status, 4))
                    gb->interrupt_flags |= INTERRUPT_LCDC;
                redraw = true;

                // Request a VBlank interrupt
                gb->interrupt_flags |= INTERRUPT_VBLANK;
                //std::cout << "requesting vblank interrupt, IE=" << std::hex << (int)gb->interrupt_enable << " IME=" << (int)gb->interrupt_master_enable << std::endl;
            } else {
                // Start rendering the next line
                mode = GPUMode::OAM;
                if (GET_BIT(lcd_status, 5))
                    gb->interrupt_flags |= INTERRUPT_LCDC;
            }
        }
        break;
    case GPUMode::VBlank:
        // During the VBlank, which lasts for the cycles equivalent
        // to 10 lines, the current line keeps increasing
        if (cycles >= T_LINE) {
            cycles -= T_LINE;
            current_line++;

            if (current_line >= PIXELS_H + 10) {
                // VBlank is finished, return to line 0
                mode = GPUMode::OAM;
                if (GET_BIT(lcd_status, 5))
                    gb->interrupt_flags |= INTERRUPT_LCDC;

                current_line = 0;
            }
        }
        break;
    }

    // Update LCD status with LY=LYC
    SET_BIT(lcd_status, 2, current_line == ly_compare);

    // If LY=LYC interrupt is enabled and LY=LYC
    if (GET_BIT(lcd_status, 6) && GET_BIT(lcd_status, 2))
        gb->interrupt_flags |= INTERRUPT_LCDC;
}

void GPU::reset() {

}

void GPU::update_tile(u16 address, u8 value) {
    // Find the vram address where this row of pixels (2 bytes) starts
    // The AND at the end sets the last bit to 0
    int address_rel = (address - 0x8000) & ~1;

    //if ((int)gb->mmu.vram[address_rel] == 0x4C)
    //    std::cout << "Read all tiles in memory" << std::endl;

    // For all 8 pixels read the color value (0-3) and store in the gpu tileset
    for (int x = 0; x < 8; x++) {
        // Mask to check bit x
        u8 mask = 1 << (7 - x);

        // The first byte in VRAM holds the LSB of the color value
        // and the second byte holds the MSB
        u8 value = ((gb->mmu.vram[address_rel]     & mask) ? 1 : 0) |
                   ((gb->mmu.vram[address_rel + 1] & mask) ? 2 : 0);

        // Multiply by 4 to go from the 2 bytes per row in vram to the
        // 8 pixels per row in the gpu tileset
        tileset[address_rel * 4 + x] = value;
    }
}

void GPU::update_object(u16 address, u8 value) {
    //std::cout << "updating object " << std::hex << (int)address << " " << (int)value << std::endl;

    int index = address >> 2;

    switch (address & 0x3) {
    case 0x0:
        sprites[index].y = value - 16;
        break;
    case 0x1:
        sprites[index].x = value - 8;
        break;
    case 0x2:
        sprites[index].tile = value;
        break;
    case 0x3:
        sprites[index].priority = (value & 0x80) != 0;
        sprites[index].y_flip   = (value & 0x40) != 0;
        sprites[index].x_flip   = (value & 0x20) != 0;
        sprites[index].palette  = (value & 0x10) != 0;
        break;
    }
}

void GPU::render_scanline() {
    if (background_enabled) {
        // The current pixel position in either the x or y direction
        // divided by 8 give the location in the background tilemap
        int tile_x = scroll_x >> 3;
        int tile_y = ((current_line + scroll_y) & 0xFF) >> 3;

        // The last 3 bits of the leftmost pixel of the current
        // scanline give the pixel coordinates in the first tile
        int pixel_x = scroll_x & 7;
        int pixel_y = (current_line + scroll_y) & 7;

        // The offset address in VRAM for either tilemap 1 or 0
        int tilemap_offset = (background_tilemap ? 0x1C00 : 0x1800);

        // Position in the screen buffer
        int canvas_offset = current_line * PIXELS_W;

        int tile = gb->mmu.vram[tilemap_offset + tile_y * 32 + tile_x];
        //std::cout << "drawing tile " << std::dec << tile << std::endl;

        //std::cout << background_tileset << " " << (tile < 128) << std::endl;
        if (!background_tileset && tile < 128)
            tile += 256;

        // Start rendering the whole scanline
        for (int i = 0; i < PIXELS_W; i++) {
            // Retrieve the pixel value from the precalculated tileset
            int value = tileset[tile * 64 + pixel_y * 8 + pixel_x];
            //std::cout << "Tile: " << std::dec << tile << " " << std::dec << pixel_x << " " << pixel_y << std::endl;

            // Extract the color from the palette
            int color = (background_palette >> (value * 2)) & 0x3;

            screen[canvas_offset] = color_palette[color];

            // Move to the next pixel
            canvas_offset++;
            pixel_x++;

            // When we reach the next tile, move to it
            if (pixel_x == 8) {
                pixel_x = 0;

                // Only use last 5 bits to stay in the range 0-31
                tile_x = (tile_x + 1) & 31;
                tile = gb->mmu.vram[tilemap_offset + tile_y * 32 + tile_x];

                if (!background_tileset && tile < 128)
                    tile += 256;
            }
        }
    }

    if (sprites_enabled) {
        // For every sprite
        for (std::size_t i = 0; i < sprites.size(); i++) {
            Sprite s = sprites[i];

            // If the sprite is at the height of the current scanline
            if (s.y + 8 >= current_line && s.y < current_line) {
                int tile_row = 0;

                // Account for the vertical flip
                if (s.y_flip)
                    tile_row = 7 - (current_line - s.y);
                else
                    tile_row = (current_line - s.y);

                tile_row -= 1;

                int canvas_offset = (current_line - 1) * PIXELS_W + s.x;
                u8 palette = s.palette ? sprite_palette_1 : sprite_palette_0;

                // For all pixels in the row
                for (int x = 0; x < 8; x++) {
                    int flipped_x = s.x_flip ? (7 - x) : x;
                    int value = tileset[s.tile * 64 + tile_row * 8 + flipped_x];

                    int index = (palette >> (value * 2)) & 0x3;

                    // If the pixel is on the screen AND
                    // it is not a transparant pixel AND
                    // the sprite has priority OR the background is transparant
                    if (s.x + x >= 0 && s.x + x < PIXELS_W && value != 0 &&
                        (!s.priority || screen[canvas_offset] == color_palette[0]))
                        screen[canvas_offset] = color_palette[index];


                    canvas_offset++;
                }
            }
        }
    }
}

u8 GPU::get_lcd_byte() {
    u8 lcd = (lcd_enabled         ? 0x80 : 0) |
             (window_tilemap      ? 0x40 : 0) |
             (window_enabled      ? 0x20 : 0) |
             (background_tileset  ? 0x10 : 0) |
             (background_tilemap  ? 0x8 : 0) |
             (sprite_size         ? 0x4 : 0) |
             (sprites_enabled     ? 0x2 : 0) |
             (background_enabled  ? 0x1 : 0);

    return lcd;
}

void GPU::set_lcd_byte(u8 lcd) {
    lcd_enabled         = (lcd & 0x80) != 0;
    window_tilemap      = (lcd & 0x40) != 0;
    window_enabled      = (lcd & 0x20) != 0;
    background_tileset  = (lcd & 0x10) != 0;
    background_tilemap  = (lcd & 0x8) != 0;
    sprite_size         = (lcd & 0x4) != 0;
    sprites_enabled     = (lcd & 0x2) != 0;
    background_enabled  = (lcd & 0x1) != 0;
}

bool GPU::get_redraw() {
    return redraw;
}

int* GPU::get_screen_buffer() {
    return &screen[0];
}

void GPU::dump_vram() {
    for (int i = 0; i < (144 / 8) * (160 / 8); i++) {
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                int value = tileset[i*64 + y*8 + x];
                u8 mask = 0x3 << (value * 2);
                int index = (background_palette & mask) >> (value * 2);

                screen[(i*8%160) + x + (y + i*8/160*8)*160] = color_palette[index];
            }
        }
    }
}