#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "ppu.h"

// ==== Inicialização e liberação ====
nes_memory_t* memory_init(nes_rom_t *rom) {
    nes_memory_t *mem = calloc(1, sizeof(nes_memory_t));
    if (!mem) return NULL;

    mem->rom = rom;              // referência à ROM completa
    mem->prg_rom = rom->prg_rom; // ponteiro para PRG-ROM
    mem->ppu = ppu_init(rom);    // inicializa PPU

    if (!mem->ppu) {
        free(mem);
        return NULL;
    }

    // Zera RAM interna
    memset(mem->ram, 0, sizeof(mem->ram));

    return mem;
}

void memory_free(nes_memory_t *mem) {
    if (!mem) return;
    if (mem->ppu) ppu_free(mem->ppu);
    free(mem);
}

// ==== Leitura de memória ====
uint8_t memory_read(nes_memory_t *mem, uint16_t addr) {
    if (addr < 0x2000) {
        // RAM interna (espelhada a cada 0x800 bytes)
        return mem->ram[addr % 0x800];
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        // PPU registers (espelhados a cada 8 bytes)
        return ppu_read(mem->ppu, 0x2000 + (addr % 8));
    }
    else if (addr >= 0x4000 && addr <= 0x4017) {
        // APU/Input registers (stub)
        return 0;
    }
    else if (addr >= 0x4020 && addr <= 0x7FFF) {
        // Expansion ROM (normalmente não usada)
        return 0;
    }
    else if (addr >= 0x8000) {
        // PRG-ROM
        if (mem->rom->prg_rom_bytes == 0x4000) {
            // ROM de 16KB → espelhada (NROM-128)
            return mem->prg_rom[(addr - 0x8000) % 0x4000];
        } else if (mem->rom->prg_rom_bytes == 0x8000) {
            // ROM de 32KB → direto
            return mem->prg_rom[addr - 0x8000];
        } else {
            printf("[MMU] Tamanho PRG-ROM inesperado: %zu bytes\n", mem->rom->prg_rom_bytes);
            return 0;
        }
    }
    else {
        // Não mapeado
        return 0;
    }
}

// ==== Escrita de memória ====
void memory_write(nes_memory_t *mem, uint16_t addr, uint8_t value) {
    if (addr < 0x2000) {
        // RAM interna (espelhada a cada 0x800 bytes)
        mem->ram[addr % 0x800] = value;
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        // PPU (espelhada a cada 8 registradores)
        ppu_write(mem->ppu, 0x2000 + (addr % 8), value);
    }
    else if (addr >= 0x4000 && addr <= 0x4017) {
        if (addr == 0x4014) {
            // DMA OAM
            printf("[MMU] DMA $%02X00 solicitado\n", value);
        }
    }
    else if (addr >= 0x4020 && addr <= 0x7FFF) {
        // Expansion ROM
        printf("[MMU] Write Expansion $%04X=%02X (ignorado)\n", addr, value);
    }
    else if (addr >= 0x8000) {
        // PRG-ROM é somente leitura
        printf("[MMU] Tentativa de escrita na ROM $%04X=%02X (ignorado)\n", addr, value);
    }
    else {
        printf("[MMU] Escrita em área não mapeada $%04X=%02X\n", addr, value);
    }
}
