#include "ppu.h"
#include <stdlib.h>
#include <string.h>

static inline uint16_t ppu_addr_increment(nes_ppu_t *ppu) {
    // PPUCTRL bit 2 define incremento: 0=+1, 1=+32
    return (ppu->PPUCTRL & 0x04) ? 32 : 1;
}

nes_ppu_t* ppu_init(void) {
    nes_ppu_t *ppu = (nes_ppu_t*)calloc(1, sizeof(nes_ppu_t));
    if (!ppu) return NULL;

    // Estado inicial razoável
    ppu->PPUSTATUS = 0x80; // finge que está em VBlank para jogos não travarem em polls de $2002
    ppu->write_latch = 0;
    ppu->vram_addr = 0;
    ppu->temp_addr = 0;
    ppu->vram_read_buffer = 0;

    // Zera memórias
    memset(ppu->oam, 0, sizeof(ppu->oam));
    memset(ppu->vram, 0, sizeof(ppu->vram));

    return ppu;
}

void ppu_free(nes_ppu_t *ppu) {
    if (!ppu) return;
    free(ppu);
}

uint8_t ppu_read(nes_ppu_t *ppu, uint16_t addr) {
    // Espera $2000-$2007; se vier com espelhos, quem chama deve normalizar
    switch (addr) {
        case 0x2002: { // PPUSTATUS (leitura)
            uint8_t value = ppu->PPUSTATUS;
            // Ler $2002 limpa VBlank (bit 7) e reseta write latch ($2005/$2006)
            ppu->PPUSTATUS &= ~0x80;
            ppu->write_latch = 0;
            return value;
        }
        case 0x2004: { // OAMDATA
            return ppu->oam[ppu->OAMADDR];
        }
        case 0x2007: { // PPUDATA (VRAM)
            uint16_t addr = ppu->vram_addr & 0x3FFF;
            uint8_t value;

            // Bufferizado para $0000-$3EFF; direto para $3F00-$3FFF (paleta)
            if (addr < 0x3F00) {
                value = ppu->vram_read_buffer;
                ppu->vram_read_buffer = ppu->vram[addr];
            } else {
                value = ppu->vram[addr];
            }

            ppu->vram_addr = (ppu->vram_addr + ppu_addr_increment(ppu)) & 0x3FFF;
            return value;
        }
        default:
            // $2000, $2001, $2003, $2005, $2006 em leitura retornam indeterminado; devolvemos 0
            return 0;
    }
}

void ppu_write(nes_ppu_t *ppu, uint16_t addr, uint8_t value) {
    switch (addr) {
        case 0x2000: // PPUCTRL
            ppu->PPUCTRL = value;
            // Bits 10-11 do temp_addr recebem nametable select (bits 0-1 do PPUCTRL)
            ppu->temp_addr = (ppu->temp_addr & 0xF3FF) | ((value & 0x03) << 10);
            break;

        case 0x2001: // PPUMASK
            ppu->PPUMASK = value;
            break;

        case 0x2003: // OAMADDR
            ppu->OAMADDR = value;
            break;

        case 0x2004: // OAMDATA
            ppu->OAMDATA = value;
            ppu->oam[ppu->OAMADDR++] = value; // auto-incrementa
            break;

        case 0x2005: // PPUSCROLL (2 writes: X, Y)
            if (ppu->write_latch == 0) {
                ppu->scroll_x = value;
                ppu->write_latch = 1;
            } else {
                ppu->scroll_y = value;
                ppu->write_latch = 0;
            }
            break;

        case 0x2006: // PPUADDR (2 writes: high, low)
            if (ppu->write_latch == 0) {
                ppu->temp_addr = (ppu->temp_addr & 0x00FF) | ((value & 0x3F) << 8);
                ppu->write_latch = 1;
            } else {
                ppu->temp_addr = (ppu->temp_addr & 0xFF00) | value;
                ppu->vram_addr = ppu->temp_addr & 0x3FFF;
                ppu->write_latch = 0;
            }
            break;

        case 0x2007: { // PPUDATA
            uint16_t addr_v = ppu->vram_addr & 0x3FFF;
            ppu->vram[addr_v] = value;
            ppu->vram_addr = (ppu->vram_addr + ppu_addr_increment(ppu)) & 0x3FFF;
            break;
        }

        default:
            // Demais endereços ($2002 leitura; outros não relevantes)
            break;
    }
}
