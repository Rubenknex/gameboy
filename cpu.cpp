#include "cpu.h"

#include <iomanip>
#include <iostream>
#include <bitset>
#include <string>
#include <regex>

#include "gameboy.h"

const std::string opcode_names[0x100] = {
    "NOP", "LD BC,d16", "LD (BC),A", "INC BC", "INC B", "DEC B", "LD B,d8", "RLCA", "LD (a16),SP", "ADD HL,BC", "LD A,(BC)", "DEC BC", "INC C", "DEC C", "LD C,d8", "RRCA",
    "STOP 0", "LD DE,d16", "LD (DE),A", "INC DE", "INC D", "DEC D", "LD D,d8", "RLA", "JR r8", "ADD HL,DE", "LD A,(DE)", "DEC DE", "INC E", "DEC E", "LD E,d8", "RRA",
    "JR NZ,r8", "LD HL,d16", "LD (HL+),A", "INC HL", "INC H", "DEC H", "LD H,d8", "DAA", "JR Z,r8", "ADD HL,HL", "LD A,(HL+)", "DEC HL", "INC L", "DEC L", "LD L,d8", "CPL",
    "JR NC,r8", "LD SP,d16", "LD (HL-),A", "INC SP", "INC (HL)", "DEC (HL)", "LD (HL),d8", "SCF", "JR C,r8", "ADD HL,SP", "LD A,(HL-)", "DEC SP", "INC A", "DEC A", "LD A,d8", "CCF",

    "LD B,B", "LD B,C", "LD B,D", "LD B,E", "LD B,H", "LD B,L", "LD B,(HL)", "LD B,A", "LD C,B", "LD C,C", "LD C,D", "LD C,E", "LD C,H", "LD C,L", "LD C,(HL)", "LD C,A",
    "LD D,B", "LD D,C", "LD D,D", "LD D,E", "LD D,H", "LD D,L", "LD D,(HL)", "LD D,A", "LD E,B", "LD E,C", "LD E,D", "LD E,E", "LD E,H", "LD E,L", "LD E,(HL)", "LD E,A",
    "LD H,B", "LD H,C", "LD H,D", "LD H,E", "LD H,H", "LD H,L", "LD H,(HL)", "LD H,A", "LD L,B", "LD L,C", "LD L,D", "LD L,E", "LD L,H", "LD L,L", "LD L,(HL)", "LD L,A",
    "LD (HL),B", "LD (HL),C", "LD (HL),D", "LD (HL),E", "LD (HL),H", "LD (HL),L", "HALT", "LD (HL),A", "LD A,B", "LD A,C", "LD A,D", "LD A,E", "LD A,H", "LD A,L", "LD A,(HL)", "LD A,A",

    "ADD A,B", "ADD A,C", "ADD A,D", "ADD A,E", "ADD A,H", "ADD A,L", "ADD A,(HL)", "ADD A,A", "ADC A,B", "ADC A,C", "ADC A,D", "ADC A,E", "ADC A,H", "ADC A,L", "ADC A,(HL)", "ADC A,A",
    "SUB B", "SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB (HL)", "SUB A", "SBC A,B", "SBC A,C", "SBC A,D", "SBC A,E", "SBC A,H", "SBC A,L", "SBC A,(HL)", "SBC A,A",
    "AND B", "AND C", "AND D", "AND E", "AND H", "AND L", "AND (HL)", "AND A", "XOR B", "XOR C", "XOR D", "XOR E", "XOR H", "XOR L", "XOR (HL)", "XOR A",
    "OR B", "OR C", "OR D", "OR E", "OR H", "OR L", "OR (HL)", "OR A", "CP B", "CP C", "CP D", "CP E", "CP H", "CP L", "CP (HL)", "CP A",

    "RET NZ", "POP BC", "JP NZ,a16", "JP a16", "CALL NZ,a16", "PUSH BC", "ADD A,d8", "RST 00H", "RET Z", "RET", "JP Z,a16", "PREFIX CB", "CALL Z,a16", "CALL a16", "ADC A,d8", "RST 08H",
    "RET NC", "POP DE", "JP NC,a16", "invalid", "CALL NC,a16", "PUSH DE", "SUB d8", "RST 10H", "RET C", "RETI", "JP C,a16", "invalid", "CALL C,a16", "invalid", "SBC A,d8", "RST 18H",
    "LDH (a8),A", "POP HL", "LD (C),A", "invalid", "invalid", "PUSH HL", "AND d8", "RST 20H", "ADD SP,r8", "JP (HL)", "LD (a16),A", "invalid", "invalid", "invalid", "XOR d8", "RST 28H",
    "LDH A,(a8)", "POP AF", "LD A,(C)", "DI", "invalid", "PUSH AF", "OR d8", "RST 30H", "LD HL,SP+r8", "LD SP,HL", "LD A,(a16)", "EI", "invalid", "invalid", "CP d8", "RST 38H",
};

const int opcode_bytes[0x100] = {
    1, 3, 1, 1, 1, 1, 2, 1, 3, 1, 1, 1, 1, 1, 2, 1,
    2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
    2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
    2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 1, 3, 3, 2, 1,
    1, 1, 3, 0, 3, 1, 2, 1, 1, 1, 3, 0, 3, 0, 2, 1,
    2, 1, 1, 0, 0, 1, 2, 1, 2, 1, 3, 0, 0, 0, 2, 1, // changed 2->1
    2, 1, 1, 1, 0, 1, 2, 1, 2, 1, 3, 1, 0, 0, 2, 1, // changed 2->1
};

const int opcode_cycles[0x100] = {
    4,  12, 8,  8,  4,  4,  8,  4,  20, 8,  8,  8, 4,  4,  8, 4,
    4,  12, 8,  8,  4,  4,  8,  4,  12, 8,  8,  8, 4,  4,  8, 4,
    8,  12, 8,  8,  4,  4,  8,  4,  8,  8,  8,  8, 4,  4,  8, 4,
    8,  12, 8,  8,  12, 12, 12, 4,  8,  8,  8,  8, 4,  4,  8, 4,
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4, 4,  4,  8, 4,
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4, 4,  4,  8, 4,
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4, 4,  4,  8, 4,
    8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4, 4,  4,  8, 4,
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4, 4,  4,  8, 4,
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4, 4,  4,  8, 4,
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4, 4,  4,  8, 4,
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4, 4,  4,  8, 4,
    8,  12, 12, 16, 12, 16, 8,  16, 8,  16, 12, 4, 12, 24, 8, 16,
    8,  12, 12, 0,  12, 16, 8,  16, 8,  16, 12, 0, 12, 0,  8, 16,
    12, 12, 8,  0,  0,  16, 8,  16, 16, 4,  16, 0, 0,  0,  8, 16,
    12, 12, 8,  4,  0,  16, 8,  16, 12, 8,  16, 4, 0,  0,  8, 16,
};

const std::string opcode_names_cb[0x100] = {
    "RLC B", "RLC C", "RLC D", "RLC E", "RLC H", "RLC L", "RLC (HL)", "RLC A", "RRC B", "RRC C", "RRC D", "RRC E", "RRC H", "RRC L", "RRC (HL)", "RRC A",
    "RL B", "RL C", "RL D", "RL E", "RL H", "RL L", "RL (HL)", "RL A", "RR B", "RR C", "RR D", "RR E", "RR H", "RR L", "RR (HL)", "RR A",
    "SLA B", "SLA C", "SLA D", "SLA E", "SLA H", "SLA L", "SLA (HL)", "SLA A", "SRA B", "SRA C", "SRA D", "SRA E", "SRA H", "SRA L", "SRA (HL)", "SRA A",
    "SWAP B", "SWAP C", "SWAP D", "SWAP E", "SWAP H", "SWAP L", "SWAP (HL)", "SWAP A", "SRL B", "SRL C", "SRL D", "SRL E", "SRL H", "SRL L", "SRL (HL)", "SRL A",

    "BIT 0,B", "BIT 0,C", "BIT 0,D", "BIT 0,E", "BIT 0,H", "BIT 0,L", "BIT 0,(HL)", "BIT 0,A", "BIT 1,B", "BIT 1,C", "BIT 1,D", "BIT 1,E", "BIT 1,H", "BIT 1,L", "BIT 1,(HL)", "BIT 1,A",
    "BIT 2,B", "BIT 2,C", "BIT 2,D", "BIT 2,E", "BIT 2,H", "BIT 2,L", "BIT 2,(HL)", "BIT 2,A", "BIT 3,B", "BIT 3,C", "BIT 3,D", "BIT 3,E", "BIT 3,H", "BIT 3,L", "BIT 3,(HL)", "BIT 3,A",
    "BIT 4,B", "BIT 4,C", "BIT 4,D", "BIT 4,E", "BIT 4,H", "BIT 4,L", "BIT 4,(HL)", "BIT 4,A", "BIT 5,B", "BIT 5,C", "BIT 5,D", "BIT 5,E", "BIT 5,H", "BIT 5,L", "BIT 5,(HL)", "BIT 5,A",
    "BIT 6,B", "BIT 6,C", "BIT 6,D", "BIT 6,E", "BIT 6,H", "BIT 6,L", "BIT 6,(HL)", "BIT 6,A", "BIT 7,B", "BIT 7,C", "BIT 7,D", "BIT 7,E", "BIT 7,H", "BIT 7,L", "BIT 7,(HL)", "BIT 7,A",

    "RES 0,B", "RES 0,C", "RES 0,D", "RES 0,E", "RES 0,H", "RES 0,L", "RES 0,(HL)", "RES 0,A", "RES 1,B", "RES 1,C", "RES 1,D", "RES 1,E", "RES 1,H", "RES 1,L", "RES 1,(HL)", "RES 1,A",
    "RES 2,B", "RES 2,C", "RES 2,D", "RES 2,E", "RES 2,H", "RES 2,L", "RES 2,(HL)", "RES 2,A", "RES 3,B", "RES 3,C", "RES 3,D", "RES 3,E", "RES 3,H", "RES 3,L", "RES 3,(HL)", "RES 3,A",
    "RES 4,B", "RES 4,C", "RES 4,D", "RES 4,E", "RES 4,H", "RES 4,L", "RES 4,(HL)", "RES 4,A", "RES 5,B", "RES 5,C", "RES 5,D", "RES 5,E", "RES 5,H", "RES 5,L", "RES 5,(HL)", "RES 5,A",
    "RES 6,B", "RES 6,C", "RES 6,D", "RES 6,E", "RES 6,H", "RES 6,L", "RES 6,(HL)", "RES 6,A", "RES 7,B", "RES 7,C", "RES 7,D", "RES 7,E", "RES 7,H", "RES 7,L", "RES 7,(HL)", "RES 7,A",

    "SET 0,B", "SET 0,C", "SET 0,D", "SET 0,E", "SET 0,H", "SET 0,L", "SET 0,(HL)", "SET 0,A", "SET 1,B", "SET 1,C", "SET 1,D", "SET 1,E", "SET 1,H", "SET 1,L", "SET 1,(HL)", "SET 1,A",
    "SET 2,B", "SET 2,C", "SET 2,D", "SET 2,E", "SET 2,H", "SET 2,L", "SET 2,(HL)", "SET 2,A", "SET 3,B", "SET 3,C", "SET 3,D", "SET 3,E", "SET 3,H", "SET 3,L", "SET 3,(HL)", "SET 3,A",
    "SET 4,B", "SET 4,C", "SET 4,D", "SET 4,E", "SET 4,H", "SET 4,L", "SET 4,(HL)", "SET 4,A", "SET 5,B", "SET 5,C", "SET 5,D", "SET 5,E", "SET 5,H", "SET 5,L", "SET 5,(HL)", "SET 5,A",
    "SET 6,B", "SET 6,C", "SET 6,D", "SET 6,E", "SET 6,H", "SET 6,L", "SET 6,(HL)", "SET 6,A", "SET 7,B", "SET 7,C", "SET 7,D", "SET 7,E", "SET 7,H", "SET 7,L", "SET 7,(HL)", "SET 7,A",
};

Register16::Register16() : upper(NULL), lower(NULL) {}

Register16::Register16(u8* upper, u8* lower) : upper(upper), lower(lower) {}

u16 Register16::get() {
    return (*upper << 8) | *lower;
}

void Register16::set(u16 value) {
    *upper = value >> 8;
    *lower = value & 0x00FF;
}

CPU::CPU(GameBoy* gb) : gb(gb) {
    counter = 0;
    cycles = 0;
    elapsed_cycles = 0;

    tracking = false;
    tracker = 0;

    halted = false;
    interrupt_master_enable = false;

    A = F = 0;
    B = C = 0;
    D = E = 0;
    H = L = 0;

    AF = Register16(&A, &F);
    BC = Register16(&B, &C);
    DE = Register16(&D, &E);
    HL = Register16(&H, &L);

    PC = 0x0;
    SP = 0x0;

    debug_printing = false;
}

CPU::~CPU() {

}

void CPU::set_flag(int flag, bool value) {
    if (value)
        F |= flag;
    else
        F &= ~flag;
}

bool CPU::get_zero() {
    return (F & FLAG_ZERO) != 0;
}

void CPU::set_zero(bool value) {
    set_flag(FLAG_ZERO, value);
}

bool CPU::get_subtract() {
    return (F & FLAG_SUBTRACT) != 0;
}

void CPU::set_subtract(bool value) {
    set_flag(FLAG_SUBTRACT, value);
}

bool CPU::get_half_carry() {
    return (F & FLAG_HALF_CARRY) != 0;
}

void CPU::set_half_carry(bool value) {
    set_flag(FLAG_HALF_CARRY, value);
}

bool CPU::get_carry() {
    return (F & FLAG_CARRY) != 0;
}

void CPU::set_carry(bool value) {
    set_flag(FLAG_CARRY, value);
}

int CPU::get_elapsed_cycles() {
    return elapsed_cycles;
}

void CPU::push_to_stack(u16 value) {
    SP -= 2;
    gb->mmu.write_word(SP, value);
}

u16 CPU::pop_from_stack() {
    u16 value = gb->mmu.read_word(SP);
    SP += 2;

    return value;
}

u8 CPU::current_opcode() {
    return gb->mmu.read_byte(PC);
}

void CPU::debug_print() {
    u8 opcode = gb->mmu.read_byte(PC);

    //if (opcode == 0)
    //    return;

    // Set up the print formatting
    std::cout << std::left << std::uppercase;
    //std::cout << std::dec << std::setw(4) << counter << " ";
    std::cout << std::hex;

    // Print the 16 bit program counter
    std::cout << std::setw(4) << PC << " ";

    // Print the 8 bit opcode and opcode name, distinguish between regular and CB prefix
    if (opcode == 0xCB) {
        u8 cb_opcode = gb->mmu.read_byte(PC + 1);
        std::cout << std::setw(2) << (int)cb_opcode << " ";
        std::cout << std::setw(11) << opcode_names_cb[cb_opcode] << " ";
    } else {
        std::cout << std::setw(2) << (int)opcode << " ";

        std::string name = opcode_names[opcode];
        if (name.find("r8") != std::string::npos) {
            int result = gb->mmu.read_byte(PC + 1);
            if (result >= 128)
                result -= 256;
            
            name.replace(name.find("r8"), 2, std::to_string(result));
        } else if (name.find("(a8)") != std::string::npos) {
            //int result = gb->mmu.read_byte(0xFF00 + gb->mmu.read_byte(PC + 1));
            //std::cout << "a8=" << (int)gb->mmu.read_byte(PC + 1) << "Value at (a8)=" << result << std::endl;
            //name.replace(name.find("r8"), 2, std::to_string(result));
        }

        std::cout << std::setw(11) << name << " ";
    }

    // if opcode name contaisn r8, replace r8 by decimal number?

    // Print the 16 bit registers
    std::cout << "AF=" << std::setw(4) << AF.get() << " ";
    std::cout << "BC=" << std::setw(4) << BC.get() << " ";
    std::cout << "DE=" << std::setw(4) << DE.get() << " ";
    std::cout << "HL=" << std::setw(4) << HL.get() << " ";

    // Print the 16 bit stack pointer
    std::cout << "SP=" << std::setw(4) << SP << " ";

    // Print the flags (bits 4-7 of register F)
    std::cout << "F=" << get_zero() << get_subtract() << get_half_carry() << get_carry() << " ";

    // Print the GPU line and cycles
    std::cout << std::hex << "LY=" << (int)gb->gpu.current_line << " ";
    std::cout << std::dec << "c=" << (int)elapsed_cycles;
    std::cout << std::endl;
}

void CPU::handle_interrupts() {
    u8 pending = gb->mmu.interrupt_enable & gb->mmu.interrupt_flags;

    if (pending) {
        halted = false;
        //std::cout << "halted=false!" << std::endl;
    }

    if (!interrupt_master_enable) {
        return;
    }

    u8 handlers[5] = {0x40, 0x48, 0x50, 0x58, 0x60};

    for (int i = 0; i < 5; i++) {
        // 1=VBLANK, 2=LCDC, 4=TIMER< 8=SERIAL, 16=JOYPAD
        u8 flag = 1 << i;

        // If this interrupt is enabled and flagged
        if (pending & flag) {
            // Reset the interrupt flag
            gb->mmu.interrupt_flags &= ~flag;
            interrupt_master_enable = false;

            // Handle the interrupt
            push_to_stack(PC);
            PC = handlers[i];
            cycles += 12;

            break;
        }
    }
}

// This function carries out all the arithmetic opcodes like ADD/ADC/SUB/etc
// immediate: if true, the value to use for the operation is given after the
void CPU::execute_ALU_opcode(u8 opcode, bool immediate) {
    u8* r[8] = {&B, &C, &D, &E, &H, &L, NULL, &A};
    int result;
    u8 value; // The value that is used, e.g. A = A + value

    u8 y = (opcode >> 3) & 0x7; // Index of half-row in 4-row block (0-7)
    u8 z = opcode & 0x7; // Index in half-row (0-7)

    if (z == 6 && !immediate) // Value at (HL)
        value = gb->mmu.read_byte(HL.get());
    else if (immediate) {// Immediate value
        value = gb->mmu.read_byte(PC + 1);
    } else // Value of a register
        value = *r[z];

    switch (y) {
    case 0: // ADD
        result = A + value;
        set_subtract(false);
        set_half_carry((result & 0x0F) < (A & 0x0F));
        set_carry(result > 0xFF);
        A = result & 0xFF;
        set_zero(A == 0);
        break;
    case 1: // ADC
        result = A + value + get_carry();
        set_subtract(false);
        // Add all lower 4 bits separately, else the carry can already occur in
        // for example (value+carry) but not in the final result
        set_half_carry(((A & 0xF) + (value & 0xF) + get_carry()) > 0xF);
        set_carry(result > 0xFF);
        A = result & 0xFF;
        set_zero(A == 0);
        break;
    case 2: // SUB
        result = A - value;
        set_subtract(true);
        set_half_carry((result & 0x0F) > (A & 0x0F));
        set_carry(result < 0);
        A = result & 0xFF;
        set_zero(A == 0);
        break;
    case 3: // SBC
        result = A - value - get_carry();
        set_subtract(true);
        set_half_carry(((A & 0xF) - (value & 0xF) - get_carry()) < 0);
        set_carry(result < 0);
        A = result & 0xFF;
        set_zero(A == 0);
        break;
    case 4: // AND
        A &= value;
        set_zero(A == 0);
        set_subtract(false); set_half_carry(true); set_carry(false);
        break;
    case 5: // XOR
        A ^= value;
        set_zero(A == 0);
        set_subtract(false); set_half_carry(false); set_carry(false);
        break;
    case 6: // OR
        A |= value;
        set_zero(A == 0);
        set_subtract(false); set_half_carry(false); set_carry(false);
        break;
    case 7: // CP
        result = A - value;
        set_zero(result == 0);
        set_subtract(true); // Changed to true
        set_half_carry((result & 0x0F) > (A & 0x0F));
        set_carry(result < 0);
        break;
    }
}

void CPU::execute_CB_opcode(u8 opcode) {
    u8* r[8] = {&B, &C, &D, &E, &H, &L, NULL, &A};
    u8 result;
    u8 value;

    u8 x = opcode >> 6;
    u8 y = (opcode >> 3) & 0x7;
    u8 z = opcode & 0x7;

    if (z == 6)
    //if (y == 6)
        value = gb->mmu.read_byte(HL.get());
    else
        value = *r[z];

    switch (x) {
    case 0:
        switch (y) {
        case 0: // RLC
            result = (value << 1) | (value >> 7);
            set_zero((result & 0xFF) == 0); set_subtract(false);
            set_half_carry(false); set_carry(value >> 7);
            break;
        case 1: // RRC
            result = (value >> 1) | (value << 7);
            set_zero(result == 0); set_subtract(false);
            set_half_carry(false); set_carry(value & 0x1);
            break;
        case 2: // RL
            result = (value << 1) | get_carry();
            set_zero(result == 0); set_subtract(false);
            set_half_carry(false); set_carry(value >> 7);
            break;
        case 3: // RR
            result = (value >> 1) | (get_carry() << 7);
            set_zero(result == 0); set_subtract(false);
            set_half_carry(false); set_carry(value & 0x1);
            break;
        case 4: // SLA
            result = value << 1;
            set_zero(result == 0); set_subtract(false);
            set_half_carry(false); set_carry(value >> 7);
            break;
        case 5: // SRA
            result = (value >> 1) | (value & 0b10000000);
            set_zero(result == 0); set_subtract(false);
            set_half_carry(false); set_carry(value & 0x1);
            break;
        case 6: // SWAP
            result = (value << 4) | (value >> 4);
            set_zero(result == 0); set_subtract(false);
            set_half_carry(false); set_carry(false);
            break;
        case 7: // SRL
            result = value >> 1;
            set_zero(result == 0); set_subtract(false);
            set_half_carry(false); set_carry(value & 0x1);
            break;
        }
        break;
    case 1: // BIT
        result = value & (1 << y);
        set_zero(result == 0); set_subtract(false); set_half_carry(true);
        break;
    case 2: // RES
        result = value & ~(1 << y);
        break;
    case 3: // SET
        result = value | (1 << y);
        break;
    }

    if (x != 1) {
        if (z == 6)
        //if (y == 6)
            gb->mmu.write_byte(HL.get(), result);
        else
            *r[z] = result;
    }
}

// Blargg test results:
// 01: DAA
// 02: EI interrupt failed
// 03: pass
// 04: pass
// 05: pass
// 06: pass
// 07: pass
// 08: pass
// 09: pass
// 10: pass
// 11: DAA
void CPU::execute_opcode() {
    u8 opcode = gb->mmu.read_byte(PC);

    // Some arrays of registers and flags which make generalizing
    // instructions easier
    u8* r[8] = {&B, &C, &D, &E, &H, &L, NULL, &A};
    Register16 rp[3] = {BC, DE, HL};
    Register16 rp2[4] = {BC, DE, HL, AF};
    bool cc[4] = {!get_zero(), get_zero(), !get_carry(), get_carry()};

    // These parameters allow grouping by positions in a 16x16 opcode table
    // by making use of the symmetry. http://www.z80.info/decoding.htm
    u8 x = opcode >> 6; // Index of 4-row block (0-3)
    u8 y = (opcode >> 3) & 0x7; // Index of half-row in 4-row block (0-7)
    u8 z = opcode & 0x7; // Index in half-row (0-7)
    u8 p = y >> 1; // Row in 4-row block (0-3)
    u8 q = y & 0x1; // Left (0) or right (1) half of row

    // Intermediate variables to store stuff during opcode execution
    u8 prev = 0;
    int result = 0;
    int a = 0;

    tracking = true;

    if (!tracking && PC == 0x1234) {
        tracker++;
        //gb->gpu.dump_vram();
    }

    //if (opcode == 0xE0)
    //    debug_printing = true;

    if (gb->debug_mode) {
        debug_print();
        //debug_printing = false;
    }

    elapsed_cycles = 0;

    // Switch per block of 4 rows
    switch (x) {
    case 0: // Rows 0-3
        // Switch per position in half-row
        switch (z) {
        case 0:
            // Switch per position of half-row in 4-row block
            switch (y) {
            case 0: // NOP
                break;
            case 1: // LD (a16), SP
                gb->mmu.write_word(gb->mmu.read_word(PC + 1), SP);
                break;
            case 2: // STOP 0

                break;
            case 3: // JR r8
                result = gb->mmu.read_byte(PC + 1);
                if (result >= 128)
                    result -= 256;
                PC += result;
                break;
            case 4: case 5: case 6: case 7: // JR cc[y-4], r8
                if (cc[y - 4]) {
                    result = gb->mmu.read_byte(PC + 1);
                    if (result >= 128)
                        result -= 256;
                    PC += result;
                    elapsed_cycles += 4;
                }
                break;
            }
            break;
        case 1:
            switch (q) {
            case 0: // LD rp[p], d16
                if (p < 3)
                    rp[p].set(gb->mmu.read_word(PC + 1));
                else
                    SP = gb->mmu.read_word(PC + 1);
                break;
            case 1: // ADD HL, rp[p]
                u16 prev = HL.get();
                int result;
                if (p < 3)
                    result = prev + rp[p].get();
                else
                    result = prev + SP;
                set_subtract(false);
                set_half_carry((result & 0xFFF) < (prev & 0xFFF));
                set_carry(result > 0xFFFF);
                HL.set(result & 0xFFFF);
                break;
            }
            break;
        case 2:
            switch (q) {
            case 0:
                switch (p) {
                case 0: // LD (BC),A
                    gb->mmu.write_byte(BC.get(), A);
                    break;
                case 1: // LD (DE),A
                    gb->mmu.write_byte(DE.get(), A);
                    break;
                case 2: // LD (HL+),A
                    gb->mmu.write_byte(HL.get(), A);
                    HL.set(HL.get() + 1);
                    break;
                case 3: // LD (HL-),A
                    gb->mmu.write_byte(HL.get(), A);
                    HL.set(HL.get() - 1);
                    break;
                }
                break;
            case 1:
                switch (p) {
                case 0: // LD A,(BC)
                    A = gb->mmu.read_byte(BC.get());
                    break;
                case 1: // LD A,(DE)
                    A = gb->mmu.read_byte(DE.get());
                    break;
                case 2: // LD A,(HL+)
                    A = gb->mmu.read_byte(HL.get());
                    HL.set(HL.get() + 1);
                    break;
                case 3: // LD A,(HL-)
                    A = gb->mmu.read_byte(HL.get());
                    HL.set(HL.get() - 1);
                    break;
                }
                break;
            }
            break;
        case 3:
            switch (q) {
            case 0: // INC rp[p]
                if (p < 3)
                    rp[p].set(rp[p].get() + 1);
                else
                    SP++;
                break;
            case 1: // DEC rp[p]
                if (p < 3)
                    rp[p].set(rp[p].get() - 1);
                else
                    SP--;
                break;
            }
            break;
        case 4: // INC r[y]
            if (y == 6) {
                prev = gb->mmu.read_byte(HL.get());
                result = prev + 1;
                gb->mmu.write_byte(HL.get(), result & 0xFF);
            } else {
                prev = *r[y];
                result = prev + 1;
                *r[y] = result & 0xFF;
            }
            set_zero((result & 0xFF) == 0);
            set_subtract(false);
            set_half_carry((result & 0xF) < (prev & 0xF));
            break;
        case 5: // DEC r[y]
            if (y == 6) {
                prev = gb->mmu.read_byte(HL.get());
                result = prev - 1;
                gb->mmu.write_byte(HL.get(), result & 0xFF);
            } else {
                prev = *r[y];
                result = prev - 1;
                *r[y] = result & 0xFF;
            }
            set_zero((result & 0xFF) == 0);
            set_subtract(true);
            set_half_carry((prev & 0xF) < 0x1);
            break;
        case 6: // LD r[y],d8
            prev = gb->mmu.read_byte(PC + 1);
            if (y == 6)
                gb->mmu.write_byte(HL.get(), prev);
            else
                *r[y] = prev;
            break;
        case 7:
            switch (y) {
            case 0: // RLCA
                set_zero(false); set_subtract(false); set_half_carry(false);
                set_carry(A >> 7);
                A = (A << 1) | (A >> 7);
                break;
            case 1: // RRCA
                set_zero(false); set_subtract(false); set_half_carry(false);
                set_carry(A & 0x1);
                A = (A >> 1) | ((A & 0x1) << 7);
                break;
            case 2: // RLA
                set_zero(false); set_subtract(false); set_half_carry(false);
                result = A << 1 | get_carry();
                set_carry(A >> 7);
                A = result & 0xFF;
                break;
            case 3: // RRA
                set_zero(false); set_subtract(false); set_half_carry(false);
                result = A >> 1 | get_carry() << 7;
                set_carry(A & 0x1);
                A = result & 0xFF;
                break;
            case 4: // DAA
                a = A;
                if (!get_subtract()) {
                    if (get_half_carry() || (a & 0xF) > 9)
                        a += 0x06;
                    if (get_carry() || a > 0x9F)
                        a += 0x60;
                } else {
                    if (get_half_carry())
                        a = (a - 6) & 0xFF;
                    if (get_carry())
                        a -= 0x60;
                }

                set_half_carry(false);

                if ((a & 0x100) == 0x100)
                    set_carry(true);

                A = a & 0xFF;
                break;
            case 5: // CPL
                A = ~A;
                set_subtract(true);
                set_half_carry(true);
                break;
            case 6: // SCF
                set_subtract(false);
                set_half_carry(false);
                set_carry(true);
                break;
            case 7: // CCF
                set_subtract(false);
                set_half_carry(false);
                set_carry(!get_carry());
                break;
            }
            break;
        }
        break;
    case 1: // Rows 4-7: LD instructions and HALT
        if (z == 6 && y == 6) { // HALT
            halted = true;
            std::cout << "halting! ime=" << interrupt_master_enable << std::endl;
        } else // LD r[y], r[z]
            if (y == 6)
                gb->mmu.write_byte(HL.get(), *r[z]);
            else if (z == 6)
                *r[y] = gb->mmu.read_byte(HL.get());
            else
                *r[y] = *r[z];
        break;
    case 2: // Rows 8-11: arithmetic functions ALU[y] r[z]
        execute_ALU_opcode(opcode, false);
        break;
    case 3: // Rows 12-15
        switch (z) {
        case 0:
            switch (y) {
            case 0: case 1: case 2: case 3: // RET cc[y]
                if (cc[y]) {
                    PC = pop_from_stack() - 1;
                    elapsed_cycles += 12;
                }
                break;
            case 4: // LDH (a8),A
                gb->mmu.write_byte(0xFF00 + gb->mmu.read_byte(PC + 1), A);
                break;
            case 5: // ADD SP,r8
                result = gb->mmu.read_byte(PC + 1);

                // Calculate two's complement of the byte
                if (result >= 128)
                    result -= 256;

                result = SP + result;

                set_zero(false);
                set_subtract(false);
                // Half carry if there is a carry from bit 3 to 4
                set_half_carry((result & 0x000F) < (SP & 0x000F));
                // Carry if there is a carry from bit 11 to 12
                set_carry((result & 0x00FF) < (SP & 0x00FF));

                SP = result & 0xFFFF;
                break;
            case 6: // LDH A,(a8)
                A = gb->mmu.read_byte(0xFF00 + gb->mmu.read_byte(PC + 1));
                break;
            case 7: // LD HL,SP+r8
                //A = gb->mmu.read_byte(gb->mmu.read_word(PC + 1));

                result = gb->mmu.read_byte(PC + 1);

                // Calculate two's complement of the byte
                if (result >= 128)
                    result -= 256;

                result = SP + result;

                set_zero(false);
                set_subtract(false);
                // Half carry if there is a carry from bit 3 to 4
                set_half_carry((result & 0x000F) < (SP & 0x000F));
                // Carry if there is a carry from bit 11 to 12
                set_carry((result & 0x00FF) < (SP & 0x00FF));

                HL.set(result & 0xFFFF);
                break;
            }
            break;
        case 1:
            switch (q) {
            case 0: // POP rp2[p]
                result = pop_from_stack();
                rp2[p].set(result);

                // When popping AF, the unused lower 4 bits of F can be set
                // so set those all to zero
                if (p == 3) {
                    F &= 0xF0;
                }
                break;
            case 1:
                switch (p) {
                case 0: // RET
                    PC = pop_from_stack() - 1;
                    break;
                case 1: // RETI
                    interrupt_master_enable = true;
                    PC = pop_from_stack() - 1;
                    break;
                case 2: // JP (HL)
                    PC = HL.get() - 1;
                    break;
                case 3: // LD SP,HL
                    SP = HL.get();
                    break;
                }
            }
            break;
        case 2: // JP cc[y],nn // LIMIT TO UPPER 4
            switch (y) {
            case 0: case 1: case 2: case 3:
                if (cc[y]) {
                    PC = gb->mmu.read_word(PC + 1) - 3;
                    elapsed_cycles += 4;
                }
                break;
            case 4: // LD (FF00+C), A
                gb->mmu.write_byte(0xFF00 + C, A);
                break;
            case 5: // LD (a16), A
                gb->mmu.write_byte(gb->mmu.read_word(PC + 1), A);
                break;
            case 6: // LD A, (FF00+C)
                A = gb->mmu.read_byte(0xFF00 + C);
                break;
            case 7: // LD A, (a16)
                A = gb->mmu.read_byte(gb->mmu.read_word(PC + 1));
                break;
            }
            break;
        case 3:
            switch (y) {
            case 0: // JP nn
                PC = gb->mmu.read_word(PC + 1) - 3;
                break;
            case 1: // CB PREFIX
                execute_CB_opcode(gb->mmu.read_byte(PC + 1));

                // Every CB instruction is length 2
                PC += 1;
                elapsed_cycles += 8;
                if (z == 0x6 || z == 0xE)
                    elapsed_cycles += 8;
                break;
            case 6: // DI
                interrupt_master_enable = false;
                break;
            case 7: // EI
                interrupt_master_enable = true;
                break;
            }
            break;
        case 4: // CALL cc[y],nn
            if (y < 4 && cc[y]) {
                push_to_stack(PC + 3);
                PC = gb->mmu.read_word(PC + 1) - 3;
                elapsed_cycles += 12;
            }
            break;
        case 5:
            switch (q) {
            case 0: // PUSH rp2[p]
                push_to_stack(rp2[p].get());
                break;
            case 1:
                if (p == 0) { // CALL nn
                    push_to_stack(PC + 3);
                    PC = gb->mmu.read_word(PC + 1) - 3;
                }
                break;
            }
            break;
        case 6: // alu[y] n
            execute_ALU_opcode(opcode, true);
            break;
        case 7: // RST y*8
            //interrupt_master_enable = false;
            push_to_stack(PC + 1);
            PC = y * 8 - 1;
            break;
        }
        break;
    }

    PC += opcode_bytes[opcode];
    elapsed_cycles += opcode_cycles[opcode];
    cycles += elapsed_cycles;
    counter++;



    float timer = ((float)cycles / (float)CLOCK_FREQ) * (float)TIMER_FREQ;
    gb->mmu.divide_register = (int)timer & 0xFF;

    // If the timer is enabled
    if (gb->mmu.timer_control & 0b100) {
        int frequencies[4] = {4096, 262144, 65536, 16384};
        int freq = frequencies[gb->mmu.timer_control & 0b11];

        gb->mmu.raw_timer_counter += (float)elapsed_cycles / (float)CLOCK_FREQ * (float)freq;
        //std::cout << "timer_counter=" << (int)gb->mmu.timer_counter << std::endl;
        
        // If we overflow the timer
        if ((int)gb->mmu.raw_timer_counter > 0xFF) {
            //std::cout << "timer_counter overflow!" << std::endl;
            gb->mmu.interrupt_flags |= INTERRUPT_TIMER;
            gb->mmu.raw_timer_counter = gb->mmu.timer_modulo;
            gb->mmu.timer_counter = gb->mmu.timer_modulo;
        } else
            gb->mmu.timer_counter = (int)gb->mmu.raw_timer_counter & 0xFF;
    }

    handle_interrupts();
}