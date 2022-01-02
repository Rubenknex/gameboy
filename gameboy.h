#ifndef GAMEBOY_H
#define GAMEBOY_H

#include <string>

#include "cartridge.h"
#include "cpu.h"
#include "gpu.h"
#include "mmu.h"

namespace Button {
	enum Type {Up, Down, Left, Right, Start, Select, A, B};
}

class GameBoy {
public:
    GameBoy(const std::string& filename);
    ~GameBoy();

    void cycle();

public:
	bool buttons[8];
    Cartridge cartridge;
    CPU cpu;
    GPU gpu;
    MMU mmu;

    bool debug_mode;
};

#endif