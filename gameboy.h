#ifndef GAMEBOY_H
#define GAMEBOY_H

#include <string>

#include "cartridge.h"
#include "cpu.h"
#include "gpu.h"
#include "mmu.h"

class GameBoy {
public:
    GameBoy(const std::string& filename);
    ~GameBoy();

    void cycle();

public:
    Cartridge cartridge;
    CPU cpu;
    GPU gpu;
    MMU mmu;

    bool interrupt_master_enable;
};

#endif