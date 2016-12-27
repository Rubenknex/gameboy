#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <string>
#include <vector>

#include "def.h"

namespace MemoryMapper {
    enum Type {None, MBC1, MBC2, MBC3, MBC4, MBC5, MMM01, HuC1};
}

namespace Destination {
    enum Type {Japanese, NonJapanese};
}

class Cartridge {
public:
    Cartridge(const std::string& filename);
    ~Cartridge();

public:
    char title[16];
    char manufacturer[4];
    char type;
    MemoryMapper::Type mapper;
    int rom_banks;
    int ram_banks;
    Destination::Type destination;

    std::vector<u8> rom_0, rom_1;
};

#endif