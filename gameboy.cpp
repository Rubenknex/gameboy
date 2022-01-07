#include "gameboy.h"

GameBoy::GameBoy(const std::string& filename) :
	buttons({false}),
    cartridge(filename),
    apu(this),
    cpu(this),
    gpu(this),
    mmu(this) {
    debug_mode = false;
}

GameBoy::~GameBoy() {

}

void GameBoy::cycle() {
    cpu.execute_opcode();
    gpu.cycle();

    for (int i = 0; i < cpu.elapsed_cycles; i++) 
        apu.cycle();
}