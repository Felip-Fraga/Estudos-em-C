#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

// Estrutura principal
typedef struct {
    uint8_t ram[0x800];   // zeropage + stack + ram
    uint8_t *prg_rom;
} nes_memory_t;

// Protótipos de funções -------------------
nes_memory_t* memory_init(uint8_t *prg_rom);
void memory_free(nes_memory_t *mem);
uint8_t memory_read(nes_memory_t *mem, uint16_t addr);
void memory_write(nes_memory_t *mem, uint16_t addr, uint8_t value);

#endif
