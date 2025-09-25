#ifndef PPU_H
#define PPU_H

#include <stdint.h>

typedef struct {
    uint8_t ctrl;      // $2000
    uint8_t mask;      // $2001
    uint8_t status;    // $2002
    uint8_t oam_addr;  // $2003
    uint8_t oam_data;  // $2004
    uint8_t scroll;    // $2005
    uint8_t addr;      // $2006
    uint8_t data;      // $2007
} nes_ppu_t;

nes_ppu_t* ppu_init();
void ppu_free(nes_ppu_t *ppu);
uint8_t ppu_read(nes_ppu_t *ppu, uint16_t addr);
void ppu_write(nes_ppu_t *ppu, uint16_t addr, uint8_t value);

#endif