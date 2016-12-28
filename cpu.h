#ifndef CPU_H
#define CPU_H

#include "def.h"

class GameBoy;

class Register16 {
public:
    Register16();
    Register16(u8* upper, u8* lower);

    u16 get();
    void set(u16 value);

private:
    u8* upper;
    u8* lower;
};

class CPU {
public:
    CPU(GameBoy* gb);
    ~CPU();

    void set_flag(int flag, bool value);

    bool get_zero();
    void set_zero(bool value);

    bool get_subtract();
    void set_subtract(bool value);

    bool get_half_carry();
    void set_half_carry(bool value);

    bool get_carry();
    void set_carry(bool value);

    int get_elapsed_cycles();

    void push_to_stack(u16 address);
    u16 pop_from_stack();

    void ALU(u8 y, u8 z, bool immediate);
    void CB_prefix(u8 x, u8 y, u8 z);

    void execute_opcode();

public:
    GameBoy* gb;

    bool tracking;
    int tracker;

    int counter;
    int cycles;
    int elapsed_cycles;

    bool interrupt_master_enable;

    u8 A, F;
    u8 B, C;
    u8 D, E;
    u8 H, L;
    Register16 AF, BC, DE, HL;
    u16 PC;
    u16 SP;
};

#endif