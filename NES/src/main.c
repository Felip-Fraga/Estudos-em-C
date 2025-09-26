#include <stdio.h>
#include <stdlib.h>
#include "rom.h"
#include "cpu.h"
#include "memory.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <arquivo.nes>\n", argv[0]);
        return 1;
    }

    // Carregar ROM
    nes_rom_t *rom = load_nes_rom(argv[1]);
    if (!rom) {
        printf("Erro ao carregar ROM!\n");
        return 1;
    }

    // Inicializar memória a partir da PRG-ROM
    nes_memory_t *memory = memory_init(rom->prg_rom);

    // Inicializar CPU
    nes_cpu_t *cpu = cpu_init(memory);

    printf("\n=== Executando ROM: %s ===\n\n", argv[1]);

    for (int i = 0; i < 100; i++) {
        if (!cpu_step(cpu, memory)) {
            printf("Execução parada.\n");
            break;
        }
    }

    printf("\nMem[0x0200] = %02X\n", memory_read(memory, 0x0200));

    // Liberar recursos
    cpu_free(cpu);
    free_nes_rom(rom);
    memory_free(memory);

    return 0;
}
