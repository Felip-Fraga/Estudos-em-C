#include "ppu.h"
#include <stdlib.h>
#include <stdio.h>

nes_ppu_t* ppu_init() {
    nes_ppu_t *ppu = calloc(1, sizeof(nes_ppu_t));
    if (!ppu) return NULL;
    ppu->status = 0x80; // NES liga com VBlank setado
    return ppu;
}

void ppu_free(nes_ppu_t *ppu) {
    if (ppu) free(ppu);
}

uint8_t ppu_read(nes_ppu_t *ppu, uint16_t addr) {
    addr &= 0x2007; // só interessa parte baixa dos endereços
    switch (addr) {
        case 0x2002: // PPUSTATUS
            return ppu->status; 
        case 0x2004: // OAMDATA
            return ppu->oam_data;
        case 0x2007: // PPUDATA
            return ppu->data;
        default:
            return 0;
    }
}

void ppu_write(nes_ppu_t *ppu, uint16_t addr, uint8_t value) {
    addr &= 0x2007;
    switch (addr) {
        case 0x2000: ppu->ctrl = value; break;
        case 0x2001: ppu->mask = value; break;
        case 0x2003: ppu->oam_addr = value; break;
        case 0x2004: ppu->oam_data = value; break;
        case 0x2005: ppu->scroll = value; break;
        case 0x2006: ppu->addr = value; break;
        case 0x2007: ppu->data = value; break;
        default: break;
    }
}