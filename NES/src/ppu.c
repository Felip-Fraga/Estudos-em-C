#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ppu.h"
#include "cpu.h"

// Framebuffer global
static uint32_t framebuffer[NES_SCREEN_HEIGHT][NES_SCREEN_WIDTH];

// Paleta oficial NES (64 cores)
static const uint32_t nes_palette[64] = {
    0x666666, 0x882A00, 0xA71214, 0xA4003C, 0x7C0074, 0x380092, 0x0010A4, 0x00206F,
    0x003400, 0x005000, 0x004C00, 0x003A00, 0x000000, 0x000000, 0x000000, 0x000000,
    0xADADAD, 0xD45528, 0xFB3C2E, 0xFF283C, 0xF5008F, 0x8D17B7, 0x1F32D2, 0x0054C6,
    0x006C28, 0x009400, 0x00A000, 0x008C00, 0x313131, 0x000000, 0x000000, 0x000000,
    0xFFFFFF, 0xFFA77A, 0xFF8277, 0xFF7787, 0xFF51C1, 0xD36CF0, 0x6092FF, 0x3EBAFF,
    0x48D374, 0x25E02C, 0x1EDF1D, 0x2AD218, 0x4E4E4E, 0x000000, 0x000000, 0x000000,
    0xFFFFFF, 0xFFCFBE, 0xFFBDB9, 0xFFBAC0, 0xFFA5DB, 0xE7B2F7, 0xB3C4FF, 0x9AD1FF,
    0xA2E0BF, 0x93E89C, 0x90E891, 0x9EE88D, 0xB2B2B2, 0x000000, 0x000000, 0x000000
};

// Variáveis SDL globais
static SDL_Window   *window   = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture  *texture  = NULL;

// ======================
// Inicialização e destruição
// ======================
nes_ppu_t* ppu_init(nes_rom_t *rom) {
    nes_ppu_t *ppu = calloc(1, sizeof(nes_ppu_t));
    if (!ppu) return NULL;

    ppu->rom = rom;

    // Inicializa arrays
    memset(ppu->vram, 0, sizeof(ppu->vram));
    memset(ppu->palette, 0, sizeof(ppu->palette));
    memset(ppu->oam, 0, sizeof(ppu->oam));

    // Inicializa registradores
    ppu->ppu_addr = 0;
    ppu->ppu_addr_latch = 0;
    ppu->ppuctrl = 0;
    ppu->ppumask = 0;
    ppu->ppustatus = 0;
    ppu->ppuscroll_x = 0;
    ppu->ppuscroll_y = 0;
    ppu->ppuscroll_latch = 0;

    // Inicializa temporização
    ppu->cycle = 0;
    ppu->scanline = 0;
    ppu->frame = 0;

    // Inicializa SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Erro ao inicializar SDL: %s\n", SDL_GetError());
        free(ppu);
        return NULL;
    }

    window = SDL_CreateWindow("NES Emulator - PPU",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        NES_SCREEN_WIDTH * 3, NES_SCREEN_HEIGHT * 3,
        0);

    if (!window) {
        printf("Erro ao criar janela: %s\n", SDL_GetError());
        free(ppu);
        return NULL;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Erro ao criar renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        free(ppu);
        return NULL;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT);

    if (!texture) {
        printf("Erro ao criar texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        free(ppu);
        return NULL;
    }

    return ppu;
}

void ppu_free(nes_ppu_t *ppu) {
    if (!ppu) return;
    
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    
    free(ppu);
}

// ======================
// Acessos CPU ↔ PPU
// ======================
uint8_t ppu_read(nes_ppu_t *ppu, uint16_t addr) {
    addr = 0x2000 + (addr % 8);

    switch (addr) {
        case 0x2002: { // PPUSTATUS
            uint8_t status = ppu->ppustatus;
            // ao ler, latch volta a 0
            ppu->ppu_addr_latch = 0;
            // VBlank flag (bit 7) é zerado ao ler
            ppu->ppustatus &= ~0x80;
            return status;
        }
        case 0x2007: { // PPUDATA
            uint8_t value = 0;
            if (ppu->ppu_addr < 0x2000) {
                // CHR-ROM
                value = ppu->rom->chr_rom[ppu->ppu_addr];
            } else if (ppu->ppu_addr >= 0x2000 && ppu->ppu_addr < 0x3F00) {
                // VRAM
                value = ppu->vram[(ppu->ppu_addr - 0x2000) % 0x800];
            } else if (ppu->ppu_addr >= 0x3F00 && ppu->ppu_addr < 0x3F20) {
                // Palette
                uint16_t pal_addr = (ppu->ppu_addr - 0x3F00) % 32;
                // Espelhamento 0x3F10/14/18/1C → 0x3F00/04/08/0C
                if ((pal_addr % 4) == 0) pal_addr &= 0x0F;
                value = ppu->palette[pal_addr];
            }
            ppu->ppu_addr++;
            return value;
        }
    }

    return 0;
}

void ppu_write(nes_ppu_t *ppu, uint16_t addr, uint8_t value) {
    addr = 0x2000 + (addr % 8);

    switch (addr) {
        case 0x2000: // PPUCTRL
            ppu->ppuctrl = value;
            break;

        case 0x2001: // PPUMASK
            ppu->ppumask = value;
            break;

        case 0x2005: // PPUSCROLL
            if (ppu->ppuscroll_latch == 0) {
                ppu->ppuscroll_x = value;
                ppu->ppuscroll_latch = 1;
            } else {
                ppu->ppuscroll_y = value;
                ppu->ppuscroll_latch = 0;
            }
            break;

        case 0x2006: // PPUADDR
            if (ppu->ppu_addr_latch == 0) {
                // primeiro write = high byte
                ppu->ppu_addr = (value & 0x3F) << 8; // só 14 bits válidos
                ppu->ppu_addr_latch = 1;
            } else {
                // segundo write = low byte
                ppu->ppu_addr |= value;
                ppu->ppu_addr &= 0x3FFF; // garante limite 14 bits
                ppu->ppu_addr_latch = 0;
            }
            break;
        
        printf("[DEBUG] Write em VRAM: addr=0x%04X valor=0x%02X\n", ppu->ppu_addr, value);
        case 0x2007: // PPUDATA
            if (ppu->ppu_addr < 0x2000) {
                // CHR-ROM é read-only
            } else if (ppu->ppu_addr >= 0x2000 && ppu->ppu_addr < 0x3F00) {
                // VRAM
                ppu->vram[(ppu->ppu_addr - 0x2000) % 0x800] = value;
            } else if (ppu->ppu_addr >= 0x3F00 && ppu->ppu_addr < 0x3F20) {
                // Palette
                uint16_t pal_addr = (ppu->ppu_addr - 0x3F00) % 32;
                if ((pal_addr % 4) == 0) pal_addr &= 0x0F;
                ppu->palette[pal_addr] = value;
            }
            ppu->ppu_addr++;
            break;

        default: break;
    }
}

// ======================
// Renderização
// ======================
void ppu_render(nes_ppu_t *ppu) {
    // === VERIFICAÇÕES DE SEGURANÇA ===
    if (!ppu) {
        printf("ERRO: ppu é NULL\n");
        return;
    }
    
    if (!ppu->rom) {
        printf("ERRO: ppu->rom é NULL\n");
        return;
    }
    
    if (!ppu->rom->chr_rom) {
        printf("ERRO: CHR-ROM não carregada\n");
        return;
    }
    
    if (!texture || !renderer) {
        printf("ERRO: SDL não inicializado (texture=%p, renderer=%p)\n", (void*)texture, (void*)renderer);
        return;
    }
    
    printf("[DEBUG] ppu_render iniciado - CHR-ROM: %zu bytes\n", ppu->rom->chr_rom_bytes);
    
    // === INICIALIZAÇÃO ===
    int tileSize = 8;
    uint8_t *chr_rom = ppu->rom->chr_rom;
    uint8_t *nametable = ppu->vram; // simplificação: mapeie VRAM direto

    // Limpa o framebuffer com cor de fundo
    uint32_t bg_color = nes_palette[ppu->palette[0] & 0x3F];
    for (int y = 0; y < NES_SCREEN_HEIGHT; y++) {
        for (int x = 0; x < NES_SCREEN_WIDTH; x++) {
            framebuffer[y][x] = bg_color;
        }
    }

    // === RENDERIZAÇÃO DOS TILES ===
    // Percorre 30 linhas × 32 colunas de tiles
    for (int ty = 0; ty < 30; ty++) {
        for (int tx = 0; tx < 32; tx++) {
            int tileIndex = nametable[ty * 32 + tx];
            uint16_t base = tileIndex * 16; // posição na CHR-ROM

            // === PROTEÇÃO CONTRA OVERFLOW ===
            if (base + 15 >= ppu->rom->chr_rom_bytes) {
                printf("AVISO: tile %d (base=0x%04X) fora do CHR-ROM (%zu bytes)\n", 
                       tileIndex, base, ppu->rom->chr_rom_bytes);
                continue; // pula este tile
            }

            // Calcula qual paleta usar (simplificado: usa paleta 0 do background)
            int paletteIndex = 0; // paletas 0-3 para background

            // === DESENHA TILE 8x8 ===
            for (int row = 0; row < 8; row++) {
                uint8_t plane1 = chr_rom[base + row];
                uint8_t plane2 = chr_rom[base + row + 8];

                for (int col = 0; col < 8; col++) {
                    int bit = 7 - col;
                    uint8_t low  = (plane1 >> bit) & 1;
                    uint8_t high = (plane2 >> bit) & 1;
                    uint8_t colorIndex = (high << 1) | low;

                    // === BUSCA COR NA PALETA ===
                    uint32_t color;
                    if (colorIndex == 0) {
                        // Cor 0 sempre usa a cor universal (0x3F00)
                        color = nes_palette[ppu->palette[0] & 0x3F];
                    } else {
                        // Cores 1-3 usam a paleta específica
                        int paletteAddr = paletteIndex * 4 + colorIndex;
                        
                        // Proteção contra overflow da paleta
                        if (paletteAddr >= 32) {
                            paletteAddr = paletteAddr % 32;
                        }
                        
                        color = nes_palette[ppu->palette[paletteAddr] & 0x3F];
                    }

                    // === CALCULA PIXEL NO FRAMEBUFFER ===
                    int px = tx * tileSize + col;
                    int py = ty * tileSize + row;
                    
                    // Proteção contra overflow do framebuffer
                    if (px >= 0 && px < NES_SCREEN_WIDTH && py >= 0 && py < NES_SCREEN_HEIGHT) {
                        framebuffer[py][px] = color;
                    }
                }
            }
        }
    }

    // === ATUALIZA TELA SDL ===
    if (SDL_UpdateTexture(texture, NULL, framebuffer, NES_SCREEN_WIDTH * sizeof(uint32_t)) != 0) {
        printf("ERRO SDL_UpdateTexture: %s\n", SDL_GetError());
        return;
    }
    
    if (SDL_RenderClear(renderer) != 0) {
        printf("ERRO SDL_RenderClear: %s\n", SDL_GetError());
        return;
    }
    
    if (SDL_RenderCopy(renderer, texture, NULL, NULL) != 0) {
        printf("ERRO SDL_RenderCopy: %s\n", SDL_GetError());
        return;
    }
    
    SDL_RenderPresent(renderer);
    
    printf("[DEBUG] ppu_render concluído com sucesso\n");
}

void ppu_render_chr_rom(nes_ppu_t *ppu, uint8_t *chr_rom) {
    int tileSize = 8;
    int tilesPerRow = 16;
    int numTiles = 256;

    for (int t = 0; t < numTiles; t++) {
        int tx = (t % tilesPerRow) * tileSize;
        int ty = (t / tilesPerRow) * tileSize;

        uint16_t base = t * 16;

        for (int row = 0; row < 8; row++) {
            uint8_t plane1 = chr_rom[base + row];
            uint8_t plane2 = chr_rom[base + row + 8];

            for (int col = 0; col < 8; col++) {
                int bit = 7 - col;
                uint8_t low = (plane1 >> bit) & 1;
                uint8_t high = (plane2 >> bit) & 1;
                uint8_t colorIndex = (high << 1) | low;

                // Usa paleta real em vez de cores fixas
                uint32_t color;
                if (colorIndex == 0) {
                    color = nes_palette[ppu->palette[0] & 0x3F];
                } else {
                    // Para debug, usa primeira paleta
                    color = nes_palette[ppu->palette[colorIndex] & 0x3F];
                }

                int px = tx + col;
                int py = ty + row;
                if (px < NES_SCREEN_WIDTH && py < NES_SCREEN_HEIGHT) {
                    framebuffer[py][px] = color;
                }
            }
        }
    }

    SDL_UpdateTexture(texture, NULL, framebuffer, NES_SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

// ======================
// Simulação de ciclos do PPU
// ======================
void ppu_step(nes_ppu_t *ppu, nes_cpu_t *cpu) {
    ppu->cycle++;

    if (ppu->cycle >= 341) {
        ppu->cycle = 0;
        ppu->scanline++;

        if (ppu->scanline == 241) {
            // início de VBlank
            ppu->ppustatus |= 0x80;
            if (ppu->ppuctrl & 0x80) {
                cpu_nmi(cpu);
            }
        }

        if (ppu->scanline >= 262) {
            // fim do frame
            ppu->scanline = 0;
            ppu->frame++;
            ppu->ppustatus &= ~0x80;
        }
    }
}
