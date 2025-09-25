#include <stdio.h>
#include "rom.h"
#include "memory.h"
#include "cpu.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <arquivo.nes>\n", argv[0]);
        return 1;
    }

    nes_rom_t *rom = load_nes_rom(argv[1]);
    if (!rom) return 1;

    nes_memory_t *mem = nes_memory_init(rom->prg_rom);
    nes_cpu_t *cpu = cpu_init(mem);

    printf("\n=== Execução inicial ===\n");
    for (int i = 0; i < 15; i++) {   // roda as 15 primeiras instruções
        cpu_step(cpu);
    }

    cpu_free(cpu);
    nes_memory_free(mem);
    free_nes_rom(rom);

    return 0;
}