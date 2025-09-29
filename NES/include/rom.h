#ifndef ROM_H
#define ROM_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// Estrutura da ROM NES
typedef struct nes_rom_t {
    uint8_t prg_rom_size;
    uint8_t chr_rom_size;
    uint8_t mapper_number;
    uint8_t mirroring;
    uint8_t has_battery;
    uint8_t has_trainer;

    uint8_t *prg_rom;
    uint8_t *chr_rom;

    size_t prg_rom_bytes;
    size_t chr_rom_bytes;
} nes_rom_t;

// Funções públicas
nes_rom_t* load_nes_rom(const char* filename);
void free_nes_rom(nes_rom_t *rom);

#endif
