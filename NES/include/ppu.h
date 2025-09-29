#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include "rom.h"

// Forward declaration para evitar dependência circular
struct nes_cpu_t;
typedef struct nes_cpu_t nes_cpu_t;

#define NES_SCREEN_WIDTH  256
#define NES_SCREEN_HEIGHT 240

typedef struct {
    nes_rom_t *rom;

    // Arrays fixos (não ponteiros!)
    uint8_t vram[0x800];    // Name tables (2 KB)
    uint8_t palette[32];    // Palette RAM (32 bytes)
    uint8_t oam[256];       // OAM (sprites)

    // Registradores PPU
    uint16_t ppu_addr;
    uint8_t  ppu_addr_latch;
    uint8_t  ppuctrl;
    uint8_t  ppumask;
    uint8_t  ppustatus;
    uint8_t  ppuscroll_x;
    uint8_t  ppuscroll_y;
    uint8_t  ppuscroll_latch;

    // Temporização
    int cycle;
    int scanline;
    int frame;
} nes_ppu_t;

// Funções
nes_ppu_t* ppu_init(nes_rom_t *rom);
void ppu_free(nes_ppu_t *ppu);

void ppu_render(nes_ppu_t *ppu);
void ppu_render_chr_rom(nes_ppu_t *ppu, uint8_t *chr_rom);

uint8_t ppu_read(nes_ppu_t *ppu, uint16_t addr);
void    ppu_write(nes_ppu_t *ppu, uint16_t addr, uint8_t value);

void ppu_step(nes_ppu_t *ppu, nes_cpu_t *cpu);

#endif
