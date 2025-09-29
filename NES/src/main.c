#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "rom.h"
#include "cpu.h"
#include "memory.h"
#include "ppu.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <rom.nes>\n", argv[0]);
        return 1;
    }

    // Carregar ROM
    nes_rom_t *rom = load_nes_rom(argv[1]);
    if (!rom) {
        printf("Erro ao carregar ROM!\n");
        return 1;
    }

    printf("ROM Info:\n");
    printf("  PRG-ROM: %d bancos (%zu bytes)\n", rom->prg_rom_size, rom->prg_rom_bytes);
    printf("  CHR-ROM: %d bancos (%zu bytes)\n", rom->chr_rom_size, rom->chr_rom_bytes);
    printf("  Mapper: %d\n", rom->mapper_number);
    printf("  Mirroring: %s\n", rom->mirroring == 0 ? "Horizontal" : "Vertical");
    printf("ROM carregada com sucesso!\n");

    // Inicializar memória e CPU
    nes_memory_t *memory = memory_init(rom);
    nes_cpu_t *cpu = cpu_init(memory);
    init_instructions(); // Inicializa tabela de instruções

    printf("[CPU] Reset concluído. PC inicial = 0x%04X\n\n", cpu->pc);
    printf("=== Executando ROM: %s ===\n\n", argv[1]);

    // ======================
    // TESTE: Popular a nametable manualmente via PPUADDR/PPUDATA
    // ======================
    printf("=== Testando escrita manual na PPU ===\n");

    uint16_t nametable_addr = 0x2000; // primeira nametable

    printf("1\n");
    
    // Preencher a nametable inteira com tile 1
    memory_write(memory, 0x2006, (nametable_addr >> 8) & 0xFF); // high byte
    memory_write(memory, 0x2006, nametable_addr & 0xFF);        // low byte
    for (int i = 0; i < 960; i++) { // 32*30 tiles
        memory_write(memory, 0x2007, 1);
    }

    printf("2\n");

    // Primeira linha com tile 2
    memory_write(memory, 0x2006, (nametable_addr >> 8) & 0xFF);
    memory_write(memory, 0x2006, nametable_addr & 0xFF);
    for (int i = 0; i < 32; i++) {
        memory_write(memory, 0x2007, 2);
    }

    printf("3\n");

    // Segunda linha com tile 3
    uint16_t second_line = nametable_addr + 32;
    memory_write(memory, 0x2006, (second_line >> 8) & 0xFF);
    memory_write(memory, 0x2006, second_line & 0xFF);
    for (int i = 0; i < 32; i++) {
        memory_write(memory, 0x2007, 3);
    }

    // Renderiza para testar
    ppu_render(memory->ppu);

    // ======================
    // Loop para manter a janela aberta
    // ======================
    SDL_Event event;
    int running = 1;
    while (running) {
        int cpu_cycles = cpu_step(cpu);        // executa 1 instrução
        int ppu_cycles = cpu_cycles * 3;       // PPU anda 3x mais rápido
        
        for (int i = 0; i < ppu_cycles; i++) {
            ppu_step(memory->ppu, cpu);        // passa ppu + cpu
        }
        
        // Renderiza 1 vez por frame
        if (memory->ppu->scanline == 0 && memory->ppu->cycle == 0) {
            ppu_render(memory->ppu);
        }
    
        // SDL eventos para fechar janela
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
        }
    }

    // Liberar recursos
    cpu_free(cpu);
    memory_free(memory);
    free_nes_rom(rom);

    return 0;
}
