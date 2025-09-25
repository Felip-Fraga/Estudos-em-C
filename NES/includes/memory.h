#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include "ppu.h"

typedef struct {
    uint8_t ram[0x0800]; // 2KB RAM (espelhada até $2000)
    uint8_t *prg_rom;    // Ponteiro para PRG-ROM
    nes_ppu_t *ppu;      // Ponteiro para PPU
} nes_memory_t;

// Funções principais
nes_memory_t* memory_init(uint8_t *prg_rom);
void memory_free(nes_memory_t *mem);

// Funções de acesso à memória
uint8_t cpu_read(nes_memory_t *mem, uint16_t addr);
void cpu_write(nes_memory_t *mem, uint16_t addr, uint8_t value);

// Aliases para compatibilidade com main.c
nes_memory_t* nes_memory_init(uint8_t *prg_rom);
void nes_memory_free(nes_memory_t *mem);

#endif