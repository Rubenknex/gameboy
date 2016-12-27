#include "gameboy.h"

GameBoy::GameBoy(const std::string& filename) :
    cartridge(filename), cpu(this), gpu(this), mmu(this, cartridge) {
    interrupt_master_enable = false;
}

GameBoy::~GameBoy() {

}

void GameBoy::cycle() {
    cpu.execute_opcode();
    gpu.cycle();
}