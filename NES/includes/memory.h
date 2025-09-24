#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdlib.h>
#include "rom.h"

// Estrutura da memória da CPU
typedef struct {
    uint8_t ram[0x0800];     // 2 KB RAM interna
    uint8_t *prg_rom;        // Programa da ROM
    size_t prg_rom_size;
} nes_memory_t;

// Inicialização
nes_memory_t* nes_memory_init(uint8_t *prg_rom, size_t prg_size);
void nes_memory_free(nes_memory_t *mem);

// Operações de leitura/escrita da CPU
uint8_t cpu_read(nes_memory_t *mem, uint16_t addr);
void cpu_write(nes_memory_t *mem, uint16_t addr, uint8_t value);

#endif