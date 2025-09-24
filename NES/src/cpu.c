#include "cpu.h"
#include <stdlib.h>
#include <stdio.h>

// ===== Funções auxiliares =====

static void set_zn_flags(nes_cpu_t *cpu, uint8_t value) {
    // Zero Flag
    if (value == 0)
        cpu->p |= FLAG_Z;
    else
        cpu->p &= ~FLAG_Z;

    // Negative Flag
    if (value & 0x80)
        cpu->p |= FLAG_N;
    else
        cpu->p &= ~FLAG_N;
}

// Lê imediato (literal)
static uint8_t immediate(nes_cpu_t *cpu) {
    return cpu_read(cpu->mem, cpu->pc++);
}

// Zero Page
static uint16_t zero_page(nes_cpu_t *cpu) {
    return cpu_read(cpu->mem, cpu->pc++);
}

// Absolute
static uint16_t absolute(nes_cpu_t *cpu) {
    uint8_t lo = cpu_read(cpu->mem, cpu->pc++);
    uint8_t hi = cpu_read(cpu->mem, cpu->pc++);
    return (hi << 8) | lo;
}

// ===== Instruções =====

// LDA
static void lda(nes_cpu_t *cpu, uint8_t value) {
    cpu->a = value;
    set_zn_flags(cpu, cpu->a);
}

// LDX
static void ldx(nes_cpu_t *cpu, uint8_t value) {
    cpu->x = value;
    set_zn_flags(cpu, cpu->x);
}

// LDY
static void ldy(nes_cpu_t *cpu, uint8_t value) {
    cpu->y = value;
    set_zn_flags(cpu, cpu->y);
}

// STA
static void sta(nes_cpu_t *cpu, uint16_t addr) {
    cpu_write(cpu->mem, addr, cpu->a);
}

// ===== Execução =====

void cpu_step(nes_cpu_t *cpu) {
    uint8_t opcode = cpu_read(cpu->mem, cpu->pc++);

    switch (opcode) {
        // ---- LDA ----
        case 0xA9: { // LDA Immediate
            lda(cpu, immediate(cpu));
        } break;

        case 0xA5: { // LDA Zero Page
            uint16_t addr = zero_page(cpu);
            lda(cpu, cpu_read(cpu->mem, addr));
        } break;

        case 0xAD: { // LDA Absolute
            uint16_t addr = absolute(cpu);
            lda(cpu, cpu_read(cpu->mem, addr));
        } break;

        // ---- LDX ----
        case 0xA2: { // LDX Immediate
            ldx(cpu, immediate(cpu));
        } break;

        // ---- LDY ----
        case 0xA0: { // LDY Immediate
            ldy(cpu, immediate(cpu));
        } break;

        // ---- STA ----
        case 0x85: { // STA Zero Page
            uint16_t addr = zero_page(cpu);
            sta(cpu, addr);
        } break;

        case 0x8D: { // STA Absolute
            uint16_t addr = absolute(cpu);
            sta(cpu, addr);
        } break;

        default:
            printf("[CPU] Opcode 0x%02X não implementado em PC=0x%04X\n", opcode, cpu->pc - 1);
            break;
    }

    // Debug
    printf("[CPU] PC=%04X A=%02X X=%02X Y=%02X SP=%02X P=%02X\n",
           cpu->pc, cpu->a, cpu->x, cpu->y, cpu->sp, cpu->p);
}