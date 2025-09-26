#ifndef PPU_H
#define PPU_H

#include <stdint.h>

typedef struct {
    // Registradores mapeados
    uint8_t PPUCTRL;    // $2000
    uint8_t PPUMASK;    // $2001
    uint8_t PPUSTATUS;  // $2002
    uint8_t OAMADDR;    // $2003
    uint8_t OAMDATA;    // $2004
    // $2005/$2006 usam latch de escrita
    uint8_t write_latch;   // 0 ou 1 para alternância
    uint8_t scroll_x;      // $2005 primeira escrita
    uint8_t scroll_y;      // $2005 segunda escrita

    // Endereçamento VRAM
    uint16_t vram_addr;    // endereço atual ($2006 concluído)
    uint16_t temp_addr;    // temporário para $2006 high/low

    // Leitura bufferizada de VRAM (comportamento real do PPU)
    uint8_t vram_read_buffer;

    // Memórias simuladas
    uint8_t oam[256];         // OAM (sprites)
    uint8_t vram[0x4000];     // VRAM simplificada (0x0000-0x3FFF)
} nes_ppu_t;

nes_ppu_t* ppu_init(void);
void ppu_free(nes_ppu_t *ppu);
uint8_t ppu_read(nes_ppu_t *ppu, uint16_t addr);    // espera $2000-$2007
void ppu_write(nes_ppu_t *ppu, uint16_t addr, uint8_t value); // espera $2000-$2007

#endif
