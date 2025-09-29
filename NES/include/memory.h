#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include "ppu.h"
#include "rom.h"

// Estrutura completa
typedef struct nes_memory_t {
    uint8_t ram[0x0800];   // 2KB de RAM
    uint8_t *prg_rom;      // Ponteiro pra PRG-ROM
    nes_rom_t *rom;        // Referência pra ROM
    nes_ppu_t *ppu;        // PPU
} nes_memory_t;

// API
nes_memory_t* memory_init(nes_rom_t *rom);
void memory_free(nes_memory_t *mem);
uint8_t memory_read(nes_memory_t *mem, uint16_t addr);
void memory_write(nes_memory_t *mem, uint16_t addr, uint8_t value);

#endif
