#include "memory.h"
#include <string.h>
#include <stdio.h>

nes_memory_t* nes_memory_init(uint8_t *prg_rom, size_t prg_size) {
    nes_memory_t *mem = malloc(sizeof(nes_memory_t));
    memset(mem->ram, 0, sizeof(mem->ram));
    mem->prg_rom = prg_rom;
    mem->prg_rom_size = prg_size;
    return mem;
}

void nes_memory_free(nes_memory_t *mem) {
    if (mem) free(mem);
}

// Leitura
uint8_t cpu_read(nes_memory_t *mem, uint16_t addr) {
    if (addr < 0x2000) {
        return mem->ram[addr % 0x0800]; // RAM com mirror
    }
    else if (addr >= 0x8000) {
        if (mem->prg_rom_size == 0x4000) {
            return mem->prg_rom[addr % 0x4000]; // NROM-128
        } else {
            return mem->prg_rom[addr - 0x8000]; // NROM-256
        }
    }
    else {
        return 0; // TODO: PPU/APU
    }
}

// Escrita
void cpu_write(nes_memory_t *mem, uint16_t addr, uint8_t value) {
    if (addr < 0x2000) {
        mem->ram[addr % 0x0800] = value;
    }
    else {
        printf("Write não implementado em 0x%04X = %02X\n", addr, value);
    }
}