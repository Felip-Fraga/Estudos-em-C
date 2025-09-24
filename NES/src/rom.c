#include "rom.h"
#include <stdio.h>
#include <string.h>

// Carrega .nes (formato iNES)
nes_rom_t* load_nes_rom(const char* filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Erro: não foi possível abrir %s\n", filename);
        return NULL;
    }

    uint8_t header[16];
    fread(header, 1, 16, file);

    if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A) {
        printf("Erro: arquivo inválido\n");
        fclose(file);
        return NULL;
    }

    nes_rom_t *rom = malloc(sizeof(nes_rom_t));
    rom->prg_rom_size = header[4];
    rom->chr_rom_size = header[5];
    rom->mapper_number = (header[7] & 0xF0) | (header[6] >> 4);
    rom->mirroring = header[6] & 0x01;
    rom->has_battery = (header[6] & 0x02) != 0;
    rom->has_trainer = (header[6] & 0x04) != 0;

    rom->prg_rom_bytes = rom->prg_rom_size * 16384;
    rom->chr_rom_bytes = rom->chr_rom_size * 8192;

    printf("ROM Info:\n");
    printf("  PRG-ROM: %d blocos (%zu bytes)\n", rom->prg_rom_size, rom->prg_rom_bytes);
    printf("  CHR-ROM: %d blocos (%zu bytes)\n", rom->chr_rom_size, rom->chr_rom_bytes);
    printf("  Mapper: %d\n", rom->mapper_number);
    printf("  Mirroring: %s\n", rom->mirroring ? "Vertical" : "Horizontal");

    // Pular trainer
    if (rom->has_trainer) fseek(file, 512, SEEK_CUR);

    rom->prg_rom = malloc(rom->prg_rom_bytes);
    fread(rom->prg_rom, 1, rom->prg_rom_bytes, file);

    if (rom->chr_rom_bytes > 0) {
        rom->chr_rom = malloc(rom->chr_rom_bytes);
        fread(rom->chr_rom, 1, rom->chr_rom_bytes, file);
    } else {
        rom->chr_rom = NULL;
    }

    fclose(file);
    printf("ROM carregada com sucesso!\n");
    return rom;
}

void free_nes_rom(nes_rom_t *rom) {
    if (!rom) return;
    free(rom->prg_rom);
    free(rom->chr_rom);
    free(rom);
}