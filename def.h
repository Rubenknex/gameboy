#ifndef DEF_H
#define DEF_H

// Set bit <n> of variable <a> to <value>
#define SET_BIT(a, n, value) (a = (a & ~(1<<(n))) | ((value)<<(n)))
#define GET_BIT(a, n) (a & (1<<(n)))

// Convenient shorthand
typedef unsigned char u8;
typedef unsigned short u16;

const int CLOCK_FREQ = 4194304;
const int TIMER_FREQ = 16384;

// Screen dimensions
const int PIXELS_W = 160;
const int PIXELS_H = 144;

// Cycles for things done for a single scanline
const int T_LINE_OAM = 80;
const int T_LINE_VRAM = 172;
const int T_HBLANK = 204;

// Total cycles for a single scanline and the entire screen
const int T_LINE = T_LINE_OAM + T_LINE_VRAM + T_HBLANK; // 456
const int T_VBLANK = 10 * T_LINE; // 4560
const int T_FULL_FRAME = PIXELS_H * T_LINE + T_VBLANK; // 70224

// Memory bank sizes
const int ROM_BANK_SIZE = 0x4000;
const int VRAM_SIZE = 0x2000;
const int ERAM_SIZE = 0x2000;
const int WRAM_SIZE = 0x2000;
const int OAM_SIZE = 0x9F;
const int HRAM_SIZE = 0x7F;

// F register flag masks
const u8 FLAG_ZERO = 0x80;
const u8 FLAG_SUBTRACT = 0x40;
const u8 FLAG_HALF_CARRY = 0x20;
const u8 FLAG_CARRY = 0x10;

// Interrupt flag masks
const u8 INTERRUPT_JOYPAD = 0x10;
const u8 INTERRUPT_SERIAL = 0x8;
const u8 INTERRUPT_TIMER = 0x4;
const u8 INTERRUPT_LCDC = 0x2;
const u8 INTERRUPT_VBLANK = 0x1;

#endif