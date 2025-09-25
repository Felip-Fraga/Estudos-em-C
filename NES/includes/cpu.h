#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "memory.h"

// Flags da CPU
#define FLAG_C 0x01
#define FLAG_Z 0x02
#define FLAG_I 0x04
#define FLAG_D 0x08   // Decimal (não usado no NES)
#define FLAG_B 0x10
#define FLAG_U 0x20   // Unused (sempre 1)
#define FLAG_V 0x40
#define FLAG_N 0x80

// Estrutura da CPU NES (6502)
typedef struct {
    uint16_t pc;     // Program Counter
    uint8_t sp;      // Stack Pointer
    uint8_t a;       // Acumulador
    uint8_t x;       // Registrador X
    uint8_t y;       // Registrador Y
    uint8_t p;       // Status Register (flags)
    nes_memory_t *mem;
} nes_cpu_t;

// === Funções públicas da CPU ===
nes_cpu_t* cpu_init(nes_memory_t *mem);
void cpu_reset(nes_cpu_t *cpu);
void cpu_free(nes_cpu_t *cpu);
void cpu_step(nes_cpu_t *cpu);

#endif