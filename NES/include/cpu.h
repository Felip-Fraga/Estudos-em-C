#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "memory.h"

// --- Flags ---
#define FLAG_C 0x01   // Carry
#define FLAG_Z 0x02   // Zero
#define FLAG_I 0x04   // Interrupt Disable
#define FLAG_D 0x08   // Decimal (não usado no NES, mas existe)
#define FLAG_B 0x10   // Break
#define FLAG_U 0x20   // Unused (sempre 1)
#define FLAG_V 0x40   // Overflow
#define FLAG_N 0x80   // Negative

// --- Enum modos de endereçamento ---
typedef enum {
    IMPLIED,
    ACCUMULATOR,
    IMMEDIATE,
    ZERO_PAGE,
    ZERO_PAGE_X,
    ZERO_PAGE_Y,
    RELATIVE,
    ABSOLUTE,
    ABSOLUTE_X,
    ABSOLUTE_Y,
    INDIRECT,
    INDIRECT_X,
    INDIRECT_Y
} addr_mode_t;

// --- Estrutura CPU ---
typedef struct nes_cpu_t {
    uint8_t a, x, y;      // registradores
    uint8_t sp;           // stack pointer
    uint16_t pc;          // program counter
    uint8_t status;       // status flags
    struct nes_memory_t *memory; // ponteiro pra memória
    
} nes_cpu_t;

// --- Estrutura de instruções ---
typedef struct {
    const char *name;
    addr_mode_t mode;
    uint8_t bytes;
    uint8_t cycles;
    void (*execute)(nes_cpu_t *cpu, addr_mode_t mode);
} instruction_t;

extern instruction_t instructions[256];

// --- API da CPU ---
nes_cpu_t* cpu_init(struct nes_memory_t *mem);
void cpu_free(nes_cpu_t *cpu);
void cpu_reset(nes_cpu_t *cpu);
int cpu_step(nes_cpu_t *cpu);
void cpu_nmi(nes_cpu_t *cpu);

// Inicializa a tabela de instruções
void init_instructions(void);

#endif
