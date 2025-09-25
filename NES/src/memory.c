#include "memory.h"
#include "ppu.h"
#include <stdlib.h>
#include <stdio.h>

// ===== Inicialização e liberação =====
nes_memory_t* memory_init(uint8_t *prg_rom) {
    nes_memory_t *mem = calloc(1, sizeof(nes_memory_t));
    if (!mem) return NULL;
    
    mem->prg_rom = prg_rom;
    mem->ppu = ppu_init();
    
    if (!mem->ppu) {
        free(mem);
        return NULL;
    }
    
    return mem;
}

void memory_free(nes_memory_t *mem) {
    if (!mem) return;
    
    ppu_free(mem->ppu);
    free(mem);
}

// ===== Leitura de memória =====
uint8_t cpu_read(nes_memory_t *mem, uint16_t addr) {
    if (addr < 0x2000) {
        // RAM (espelhada a cada 0x800 bytes)
        return mem->ram[addr % 0x0800];
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        // PPU registers (espelhados a cada 8 bytes)
        return ppu_read(mem->ppu, 0x2000 + (addr % 8));
    }
    else if (addr >= 0x4000 && addr <= 0x4017) {
        // APU/Input registers (stub por enquanto)
        return 0;
    }
    else if (addr >= 0x4020 && addr <= 0x7FFF) {
        // Expansion ROM (não usado na maioria dos jogos)
        return 0;
    }
    else if (addr >= 0x8000) {
        // PRG-ROM
        return mem->prg_rom[addr - 0x8000];
    }
    else {
        // Área não mapeada
        return 0;
    }
}

// ===== Escrita de memória =====
void cpu_write(nes_memory_t *mem, uint16_t addr, uint8_t value) {
    if (addr < 0x2000) {
        // RAM (espelhada a cada 0x800 bytes)
        mem->ram[addr % 0x0800] = value;
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        // PPU registers (espelhados a cada 8 bytes)
        ppu_write(mem->ppu, 0x2000 + (addr % 8), value);
    }
    else if (addr >= 0x4000 && addr <= 0x4017) {
        // APU/Input registers (stub por enquanto)
        if (addr == 0x4014) {
            // DMA transfer (por enquanto só ignora)
            printf("[MMU] DMA transfer solicitado: $%02X00\n", value);
        }
        // Outros registradores APU são ignorados por enquanto
    }
    else if (addr >= 0x4020 && addr <= 0x7FFF) {
        // Expansion ROM (não usado na maioria dos jogos)
        printf("[MMU] Write em Expansion ROM $%04X = %02X (ignorado)\n", addr, value);
    }
    else if (addr >= 0x8000) {
        // Tentativa de escrita em ROM
        printf("[MMU] Tentativa de escrita em ROM $%04X = %02X (ignorado)\n", addr, value);
    }
    else {
        // Área não mapeada
        printf("[MMU] Write em área não mapeada $%04X = %02X\n", addr, value);
    }
}

// ===== Aliases para compatibilidade =====
nes_memory_t* nes_memory_init(uint8_t *prg_rom) {
    return memory_init(prg_rom);
}

void nes_memory_free(nes_memory_t *mem) {
    memory_free(mem);
}