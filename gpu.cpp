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

    scroll_x = scroll_y = 0;
    current_line = 0;
    background_palette = 0;

    color_palette[0] = 0xFFFFFFFF;
    color_palette[1] = 0xAAAAAAFF;
    color_palette[2] = 0x555555FF;
    color_palette[3] = 0x000000FF;

    redraw = false;

    screen.resize(PIXELS_W * PIXELS_H);
    std::fill(screen.begin(), screen.end(), color_palette[0]);

    // 256+128=384 unique tiles, each consisting of 8*8 pixels
    tileset.resize((256 + 128) * 8 * 8);
}

void GPU::cycle() {
    redraw = false;

    cycles += gb->cpu.get_elapsed_cycles();
    //std::cout << "CYCLES: " << cycles << std::endl;

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
                redraw = true;

                // Request a VBlank interrupt
                gb->mmu.interrupt_flags |= INTERRUPT_VBLANK;
                //std::cout << "requesting vblank interrupt, IE=" << std::hex << (int)gb->mmu.interrupt_enable << " IME=" << (int)gb->interrupt_master_enable << std::endl;
            } else {
                // Start rendering the next line
                mode = GPUMode::OAM;
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
                current_line = 0;
            }
        }
        break;
    }
}

void GPU::reset() {

}

void GPU::update_tile(u16 address, u8 value) {
    // Find the vram address where this row of pixels (2 bytes) starts
    // The AND at the end sets the last bit to 0
    int address_rel = (address - 0x8000) & ~1;

    if ((int)gb->mmu.vram[address_rel] == 0x4C)
        std::cout << "Read all tiles in memory" << std::endl;

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

void GPU::render_scanline() {
    // Only render background if it is set to ON
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

        //if (background_tileset && tile < 128)
        //    tile += 255;

        // Start rendering the whole scanline
        for (int i = 0; i < PIXELS_W; i++) {
            // Retrieve the pixel value from the precalculated tileset
            int value = tileset[tile * 64 + pixel_y * 8 + pixel_x];
            //std::cout << "Tile: " << std::dec << tile << " " << std::dec << pixel_x << " " << pixel_y << std::endl;

            // Mask to extract the palette color belonging to the color value
            u8 mask = 0x3 << (value * 2);
            int index = (background_palette & mask) >> (value * 2);

            // Set the color of the screen pixel
            screen[canvas_offset] = color_palette[index];

            // Move to the next pixel
            canvas_offset++;
            pixel_x++;

            // When we reach the next tile, move to it
            if (pixel_x == 8) {
                pixel_x = 0;

                // Only use last 5 bits to stay in the range 0-31
                tile_x = (tile_x + 1) & 31;
                tile = gb->mmu.vram[tilemap_offset + tile_y * 32 + tile_x];

                //if (background_tileset && tile < 128)
                //    tile += 255;
            }
        }
    }
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