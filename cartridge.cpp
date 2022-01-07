#include "cartridge.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include "fmt/format.h"

/*

No MBC: Single 32 KiB ROM bank (size 0x8000)
MBC1: Max 2 MB ROM (128*16 KiB) and/or 32 KiB RAM
*/

Cartridge::Cartridge(const std::string& filename) {
    std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
    
    if (!file.is_open()) {
        std::cout << "Cannot open the file: " << filename << std::endl;
        return;
    }
    
    std::cout << fmt::format("Loading cartridge: {0}", filename) << std::endl;

    // Read the header info
    file.seekg(0x134);
    file.read(title, 16);
    file.read(manufacturer, 4);

    file.seekg(0x147);
    file.read(&type, 1);
    if (type == 0x0 || type == 0x8 || type == 0x9)
        mapper = MemoryMapper::None;
    else if (type == 0x1 || type == 0x2 || type == 0x3)
        mapper = MemoryMapper::MBC1;
    else if (type == 0x5 || type == 0x6)
        mapper = MemoryMapper::MBC2;
    else if (type == 0xB || type == 0xC || type == 0xD)
        mapper = MemoryMapper::MMM01;
    else if (type == 0xF || type == 0x10 || type == 0x11 || type == 0x12 || type == 0x13)
        mapper = MemoryMapper::MBC3;
    else if (type == 0x15 || type == 0x16 || type == 0x17)
        mapper = MemoryMapper::MBC4;
    else if (type == 0x19 || type == 0x1A || type == 0x1B || type == 0x1C || type == 0x1D || type == 0x1E)
        mapper = MemoryMapper::MBC5;
    else if (type == 0xFE)
        mapper = MemoryMapper::HuC1;
    else
        std::cout << fmt::format("Error: Unknown memory mapper type: {0:02X}", type) << std::endl;

    char rom_size, ram_size;
    file.read(&rom_size, 1);
    rom_banks = 2 << rom_size;
    file.read(&ram_size, 1);
    ram_banks = ram_size;

    char dest;
    file.read(&dest, 1);
    destination = (Destination::Type)dest;

    // Read the ROM banks
    file.seekg(0x0);
    rom.resize(rom_banks * ROM_BANK_SIZE);
    file.read((char*) &rom[0], rom_banks * ROM_BANK_SIZE);
    //rom_0.resize(ROM_BANK_SIZE);
    //file.read((char*) &rom_0[0], ROM_BANK_SIZE);
    //rom_1.resize(ROM_BANK_SIZE);
    //file.read((char*) &rom_1[0], ROM_BANK_SIZE);

    // TODO: Read more banks if other memory mappers

    std::cout << "Title: " << title << std::endl;
    std::cout << "Manufacturer: " << manufacturer << std::endl;
    std::cout << std::hex;
    std::cout << "Cartridge Type: " << (int)type << std::endl;
    std::cout << std::dec;
    std::cout << "ROM Banks: " << rom_banks << std::endl;
    std::cout << "RAM Banks: " << ram_banks << std::endl;
}

Cartridge::~Cartridge() {

}