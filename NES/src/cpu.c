#include <stdlib.h>
#include <stdio.h>
#include "cpu.h"
#include "memory.h"

#define DEBUG_CPU 1   // 0 = off | 1 = on

// ============================ Helpers ============================

// Define/inclui ou limpa uma flag
static inline void set_flag(nes_cpu_t *cpu, uint8_t flag, int enabled) {
    if (enabled) cpu->status |= flag;
    else cpu->status &= ~flag;
}

// Retorna valor de uma flag
static inline uint8_t get_flag(nes_cpu_t *cpu, uint8_t flag) {
    return (cpu->status & flag) ? 1 : 0;
}

// Fetch helpers
static inline uint8_t fetch8(nes_cpu_t *cpu) {
    return memory_read(cpu->memory, cpu->pc++);
}
static inline uint16_t fetch16(nes_cpu_t *cpu) {
    uint8_t lo = fetch8(cpu);
    uint8_t hi = fetch8(cpu);
    return (hi << 8) | lo;
}

// Addressing modes (principais)
static inline uint16_t addr_immediate(nes_cpu_t *cpu) { return cpu->pc++; }
static inline uint16_t addr_zero_page(nes_cpu_t *cpu) { return fetch8(cpu); }
static inline uint16_t addr_zero_page_x(nes_cpu_t *cpu) { return (fetch8(cpu) + cpu->x) & 0xFF; }
static inline uint16_t addr_absolute(nes_cpu_t *cpu) { return fetch16(cpu); }
static inline uint16_t addr_absolute_x(nes_cpu_t *cpu) { return fetch16(cpu) + cpu->x; }
static inline uint16_t addr_absolute_y(nes_cpu_t *cpu) { return fetch16(cpu) + cpu->y; }

// ============================ Init / Reset ============================

nes_cpu_t* cpu_init(nes_memory_t *memory) {
    nes_cpu_t *cpu = calloc(1, sizeof(nes_cpu_t));
    cpu->memory = memory;   // <<=== importante!
    cpu->sp = 0xFD;
    cpu->status = 0x24;
    // PC inicial do Reset
    uint8_t lo = memory_read(memory, 0xFFFC);
    uint8_t hi = memory_read(memory, 0xFFFD);
    cpu->pc = (hi << 8) | lo;
    return cpu;
}
void cpu_free(nes_cpu_t *cpu) {
    free(cpu);
}

void cpu_reset(nes_cpu_t *cpu) {
    // Registradores A, X, Y ficam indefinidos no reset real
    // mas por compatibilidade, vamos zerar
    cpu->a = 0;
    cpu->x = 0; 
    cpu->y = 0;
    
    // Stack pointer começa em 0xFD
    cpu->sp = 0xFD;
    
    // Status: I flag setada (IRQs desabilitados), unused bit sempre 1
    cpu->status = 0x24;  // 00100100 = I flag + unused bit
    
    // PC = vetor de reset ($FFFC/$FFFD)
    uint8_t lo = memory_read(cpu->memory, 0xFFFC);
    uint8_t hi = memory_read(cpu->memory, 0xFFFD);
    cpu->pc = ((uint16_t)hi << 8) | lo;
    
    printf("[CPU] Reset concluído. PC inicial = 0x%04X\n", cpu->pc);
}
// ============================ Execução ============================

int cpu_step(nes_cpu_t *cpu) {
    uint8_t opcode = memory_read(cpu->memory, cpu->pc++);
    instruction_t inst = instructions[opcode];
    
    if (inst.execute == NULL) {
        printf("[CPU] ERRO: Opcode 0x%02X não implementado em PC=0x%04X\n", opcode, cpu->pc - 1);
        return 2;
    }

    inst.execute(cpu, inst.mode);
    return inst.cycles;
}

void cpu_nmi(nes_cpu_t *cpu) {
    uint16_t pc = cpu->pc;

    // --- empilha PC ---
    // memory_write(cpu->memory, 0x0100 + cpu->sp--, (pc >> 8) & 0xFF); // hi
    // memory_write(cpu->memory, 0x0100 + cpu->sp--, pc & 0xFF);        // lo
    memory_write(cpu->memory, 0x0100 + cpu->sp--, (pc >> 8) & 0xFF); // hi
    memory_write(cpu->memory, 0x0100 + cpu->sp--, pc & 0xFF);        // lo


    // --- empilha status ---
    uint8_t flags = cpu->status;
    flags &= ~0x10;  // limpa bit de BRK (não faz parte do push automático em NMI/IRQ)
    flags |= 0x20;   // seta bit "unused"
    memory_write(cpu->memory, 0x0100 + cpu->sp--, flags);

    // --- pega novo PC do vetor de NMI ($FFFA/B) ---
    uint8_t lo = memory_read(cpu->memory, 0xFFFA);
    uint8_t hi = memory_read(cpu->memory, 0xFFFB);
    cpu->pc = (hi << 8) | lo;

    // --- seta flag de interrupção ---
    cpu->status |= 0x04; // set I (disable IRQs durante execução da NMI)
}
