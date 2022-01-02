#include "mmu.h"

#include <iostream>

#include "gameboy.h"

const u8 bios[0x100] = {
    0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
    0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
    0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
    0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
    0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
    0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
    0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
    0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
    0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xF2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
    0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
    0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3c, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x4C,
    0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
    0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50
};

MMU::MMU(GameBoy* gb, const Cartridge& cartridge) : gb(gb), rom_0(cartridge.rom_0), rom_1(cartridge.rom_1) {
    in_bios = true;

    vram.resize(VRAM_SIZE);
    eram.resize(ERAM_SIZE);
    wram.resize(WRAM_SIZE);
    oam.resize(OAM_SIZE);
    hram.resize(HRAM_SIZE);

    select_button = false;
    select_direction = false;
    down_or_start = true;
    up_or_select = true;
    left_or_b = true;
    right_or_a = true;

    divide_register = 0;
    timer_counter = 0;
    timer_modulo = 0;
    timer_control = 0;

    interrupt_flags = 0;
    interrupt_enable = 0;
}

MMU::~MMU() {

}

u8 MMU::read_byte(u16 address) {
    u8 result = 0;

    if (address < 0x4000) {
        // Instead of swapping the first 100 bytes when the end of the
        // bios is reached, use a simple flag
        if (in_bios) {
            if (address < 0x100)
                result = bios[address];
            else if (gb->cpu.PC == 0x100)
                in_bios = false;
        }

        return rom_0[address];
    } else if (address < 0x8000)
        result = rom_1[address & 0x3FFF];
    else if (address < 0xA000)
        result = vram[address & 0x1FFF];
    else if (address < 0xC000)
        result = eram[address & 0x1FFF];
    //else if (address < 0xE000)
    //    return wram[address & 0x1FFF];
    else if (address < 0xFE00)
        result = wram[address & 0x1FFF];
    else if (address < 0xFF00) {
        if (address < 0xFEA0)
            result = oam[address & 0xFF];
    } else if (address < 0xFF80)
        switch (address & 0xFF) {
        case 0x00: { // Joypad port
            // Construct the joypad register byte
            // the selection bits are swapped for some reason
            if (select_direction)
                result = (!gb->buttons[Button::Start]  ? 0x8 : 0) |
                         (!gb->buttons[Button::Select] ? 0x4 : 0) |
                         (!gb->buttons[Button::A]      ? 0x2 : 0) |
                         (!gb->buttons[Button::B]      ? 0x1 : 0);
            else if (select_button)
                result = (!gb->buttons[Button::Down]   ? 0x8 : 0) |
                         (!gb->buttons[Button::Up]     ? 0x4 : 0) |
                         (!gb->buttons[Button::Left]   ? 0x2 : 0) |
                         (!gb->buttons[Button::Right]  ? 0x1 : 0);
            else
                return 0;
            } break;
        case 0x01: // Serial IO data

            break;
        case 0x02: // Serial IO control

            break;
        case 0x04: // Divider
            // Temporary fix, return a random number
            //result = (u8)rand();
            result = divide_register;
            break;
        case 0x05: // Timer counter

            break;
        case 0x06: // Timer modulo

            break;
        case 0x07: // Timer control

            break;
        case 0x0F: // Interrupt flags
            result = interrupt_flags;
            break;
        case 0x10: // Sound mode 1 sweep

            break;
        case 0x11: // Sound mode 1 length/wave pattern duty

            break;
        case 0x12: // Sound mode 1 envelope

            break;
        case 0x13: // Sound mode 1 frequency low

            break;
        case 0x14: // Sound mode 1 frequency high

            break;
        case 0x16: // Sound mode 2 length/wave pattern duty

            break;
        case 0x17: // Sound mode 2 envelope

            break;
        case 0x18: // Sound mode 2 frequency low

            break;
        case 0x19: // Sound mode 2 frequency high

            break;
        case 0x1A: // Sound mode 3 on/off

            break;
        case 0x1B: // Sound mode 3 length

            break;
        case 0x1C: // Sound mode 3 output level

            break;
        case 0x1D: // Sound mode 3 frequency low

            break;
        case 0x1E: // Sound mode 3 frequency high

            break;
        case 0x20: // Sound mode 4 length

            break;
        case 0x21: // Sound mode 4 envelope

            break;
        case 0x22: // Sound mode 4 polynomial counter

            break;
        case 0x23: // Sound mode 4 counter/consecutive

            break;
        case 0x24: // Channel control

            break;
        case 0x25: // Sound output terminal

            break;
        case 0x26: // Sound on/off

            break;
        case 0x30: // Waveform storage5

            break;
        case 0x40: // LCD Control
            // Construct the LCD control byte from the gpu parameters
            result = gb->gpu.get_lcd_byte();
            break;
        case 0x41: // LCD Status

            break;
        case 0x42: // Scroll Y
            result = gb->gpu.scroll_y;
            break;
        case 0x43: // Scroll X
            result = gb->gpu.scroll_x;
            break;
        case 0x44: // Y Coord
            result = gb->gpu.current_line;
            break;
        case 0x45: // LY compare

            break;
        case 0x46: // DMA transfer start address

            break;
        case 0x47: // BG and window palette data
            result = gb->gpu.background_palette;
            break;
        case 0x48: // Object palette 0 data
            result = gb->gpu.sprite_palette_0;
            break;
        case 0x49: // Object palette 1 data
            result = gb->gpu.sprite_palette_1;
            break;
        case 0x4A: // Window Y

            break;
        case 0x4B: // Window X

            break;
        default: {
            result = 0;
        }
        }
    else if (address < 0xFFFF) {
        result = hram[address & 0x7F];
    } else if (address == 0xFFFF)
        result = interrupt_enable;

    return result;
}

void MMU::write_byte(u16 address, u8 value) {
    if (address < 0x4000)
        rom_0[address] = value;
    else if (address < 0x8000)
        rom_1[address & 0x3FFF] = value;
    else if (address < 0xA000) {
        vram[address & 0x1FFF] = value;
        if (address < 0x9800)
            gb->gpu.update_tile(address, value);
    } else if (address < 0xC000)
        eram[address & 0x1FFF] = value;
    //else if (address < 0xE000)
    //    wram[address & 0x1FFF] = value;
    else if (address < 0xFE00)
        wram[address & 0x1FFF] = value;
    else if (address < 0xFF00) {
        // Graphics sprite information
        if (address < 0xFEA0) {
            oam[address & 0xFF] = value;
            gb->gpu.update_object(address - 0xFE00, value);
        }
    } else if (address < 0xFF80)
        switch (address & 0xFF) {
        case 0x00: // Joypad port
            select_button    = (value & 0x20) != 0;
            select_direction = (value & 0x10) != 0;
            break;
        case 0x01: // Serial IO data

            break;
        case 0x02: // Serial IO control

            break;
        case 0x04: // Divider

            break;
        case 0x05: // Timer counter

            break;
        case 0x06: // Timer modulo

            break;
        case 0x07: // Timer control

            break;
        case 0x0F: // Interrupt flags
            interrupt_flags = value;
            break;
        case 0x10: // Sound mode 1 sweep

            break;
        case 0x11: // Sound mode 1 length/wave pattern duty

            break;
        case 0x12: // Sound mode 1 envelope

            break;
        case 0x13: // Sound mode 1 frequency low

            break;
        case 0x14: // Sound mode 1 frequency high

            break;
        case 0x16: // Sound mode 2 length/wave pattern duty

            break;
        case 0x17: // Sound mode 2 envelope

            break;
        case 0x18: // Sound mode 2 frequency low

            break;
        case 0x19: // Sound mode 2 frequency high

            break;
        case 0x1A: // Sound mode 3 on/off

            break;
        case 0x1B: // Sound mode 3 length

            break;
        case 0x1C: // Sound mode 3 output level

            break;
        case 0x1D: // Sound mode 3 frequency low

            break;
        case 0x1E: // Sound mode 3 frequency high

            break;
        case 0x20: // Sound mode 4 length

            break;
        case 0x21: // Sound mode 4 envelope

            break;
        case 0x22: // Sound mode 4 polynomial counter

            break;
        case 0x23: // Sound mode 4 counter/consecutive

            break;
        case 0x24: // Channel control

            break;
        case 0x25: // Sound output terminal

            break;
        case 0x26: // Sound on/off

            break;
        case 0x30: // Waveform storage

            break;
        case 0x40: // LCD Control
            gb->gpu.set_lcd_byte(value);
            break;
        case 0x41: // LCD Status

            break;
        case 0x42: // Scroll Y
            gb->gpu.scroll_y = value;
            break;
        case 0x43: // Scroll X
            gb->gpu.scroll_x = value;
            break;
        case 0x45: // LY compare

            break;
        case 0x46: {// DMA transfer start address
            u16 start_addr = (value << 8);

            for (int i = 0; i < 0x9F; i++)
                write_byte(0xFE00 + i, read_byte(start_addr + i));
            } break;
        case 0x47: // BG and window palette data
            gb->gpu.background_palette = value;
            //std::cout << "Set palette: " << std::hex << (int)value << std::endl;
            break;
        case 0x48: // Object palette 0 data
            gb->gpu.sprite_palette_0 = value;
            break;
        case 0x49: // Object palette 1 data
            gb->gpu.sprite_palette_1 = value;
            break;
        case 0x4A: // Window Y

            break;
        case 0x4B: // Window X

            break;
        }
    else if (address < 0xFFFF)
        hram[address & 0x7F] = value;
    else if (address == 0xFFFF)
        interrupt_enable = value;
}

u16 MMU::read_word(u16 address) {
    return (read_byte(address + 1) << 8) | read_byte(address);
}

void MMU::write_word(u16 address, u16 value) {
    write_byte(address, value & 0x00FF);
    write_byte(address + 1, value >> 8);
}