#include "gameboy.h"

GameBoy::GameBoy(const std::string& filename) :
	buttons({false}),
    cartridge(filename),
    cpu(this),
    gpu(this),
    mmu(this, cartridge) {

}

GameBoy::~GameBoy() {

}

void GameBoy::cycle() {
    cpu.execute_opcode();
    gpu.cycle();
}