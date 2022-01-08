#include "gameboy.h"

GameBoy::GameBoy(const std::string& filename) :
	buttons({false}),
    cartridge(filename),
    apu(this),
    cpu(this),
    gpu(this),
    mmu(this) {
    select_button = false;
    select_direction = false;
    down_or_start = true;
    up_or_select = true;
    left_or_b = true;
    right_or_a = true;

    divide_register = 0;
    raw_timer_counter = 0;
    timer_counter = 0;
    timer_modulo = 0;
    timer_control = 0;

    disable_bios = 0;

    interrupt_flags = 0;
    interrupt_enable = 0;

    debug_mode = false;
}

GameBoy::~GameBoy() {

}

u8 GameBoy::read_byte(u16 address) {
    u8 result = 0;
    
    switch (address & 0xFF) {
    case 0x00: { // Joypad port
        // Construct the joypad register byte
        // the selection bits are swapped for some reason
        if (select_direction)
            result = (!buttons[Button::Start]  ? 0x8 : 0) |
                     (!buttons[Button::Select] ? 0x4 : 0) |
                     (!buttons[Button::A]      ? 0x2 : 0) |
                     (!buttons[Button::B]      ? 0x1 : 0);
        else if (select_button)
            result = (!buttons[Button::Down]   ? 0x8 : 0) |
                     (!buttons[Button::Up]     ? 0x4 : 0) |
                     (!buttons[Button::Left]   ? 0x2 : 0) |
                     (!buttons[Button::Right]  ? 0x1 : 0);
        else
            return 0;
        } break;
    case 0x01: // Serial IO data

        break;
    case 0x02: // Serial IO control

        break;
    case 0x04: // Divider
        result = divide_register;
        break;
    case 0x05: // Timer counter
        result = timer_counter;
        break;
    case 0x06: // Timer modulo
        result = timer_modulo;
        break;
    case 0x07: // Timer control
        result = timer_control;
        break;
    case 0x0F: // Interrupt flags
        result = interrupt_flags;
        break;
    }

    return result;
}

void GameBoy::write_byte(u16 address, u8 value) {
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
        divide_register = 0;
        break;
    case 0x05: // Timer counter
        timer_counter = value;
        break;
    case 0x06: // Timer modulo
        timer_modulo = value;
        //std::cout << std::hex << "setting timer_modulo=" << (int)timer_modulo << std::endl;
        break;
    case 0x07: // Timer control
        timer_control = value;
        //std::cout << std::hex << "setting timer_control=" << (int)timer_control << std::endl;
        break;
    case 0x0F: // Interrupt flags
        interrupt_flags = value;
        break;
    }
}

void GameBoy::cycle() {
    cpu.execute_opcode();

    gpu.cycle();

    // APU should run at 4 MHz
    for (int i = 0; i < cpu.elapsed_cycles; i++) 
        apu.cycle();
}