#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "rom.h"
#include "cpu.h"
#include "memory.h"
#include "ppu.h"

// ==== Inicialização/Reset/Liberação da CPU ====
nes_cpu_t *cpu_init(nes_memory_t *mem)
{
    nes_cpu_t *cpu = malloc(sizeof(nes_cpu_t));
    if (!cpu) return NULL;

    cpu->mem = mem;
    cpu_reset(cpu);
    return cpu;
}

void cpu_reset(nes_cpu_t *cpu)
{
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    cpu->sp = 0xFD;
    cpu->p = FLAG_U | FLAG_I;

    // vetor reset: endereço inicial está em 0xFFFC-0xFFFD
    uint8_t lo = memory_read(cpu->mem, 0xFFFC);
    uint16_t hi = memory_read(cpu->mem, 0xFFFD);
    cpu->pc = (hi << 8) | lo;

    printf("[CPU] Reset concluído. PC inicial = 0x%04X\n", cpu->pc);
}

void cpu_free(nes_cpu_t *cpu)
{
    if (cpu) free(cpu);
}

static void set_zn_flags(nes_cpu_t *cpu, uint8_t value)
{
    if (value == 0)
        cpu->p |= FLAG_Z;
    else
        cpu->p &= ~FLAG_Z;

    if (value & 0x80)
        cpu->p |= FLAG_N;
    else
        cpu->p &= ~FLAG_N;
}

// ==== Funções auxiliares de endereçamento ====
static uint8_t immediate(nes_cpu_t *cpu)
{
    return memory_read(cpu->mem, cpu->pc++);
}

static uint16_t zero_page(nes_cpu_t *cpu)
{
    return memory_read(cpu->mem, cpu->pc++);
}

static uint16_t absolute(nes_cpu_t *cpu)
{
    uint8_t lo = memory_read(cpu->mem, cpu->pc++);
    uint8_t hi = memory_read(cpu->mem, cpu->pc++);
    return (hi << 8) | lo;
}

static uint16_t zero_page_x(nes_cpu_t *cpu) {
    uint8_t addr = (memory_read(cpu->mem, cpu->pc++) + cpu->x) & 0xFF;
    return addr;
}

static uint16_t zero_page_y(nes_cpu_t *cpu) {
    uint8_t addr = (memory_read(cpu->mem, cpu->pc++) + cpu->y) & 0xFF;
    return addr;
}

static uint16_t absolute_x(nes_cpu_t *cpu) {
    uint8_t lo = memory_read(cpu->mem, cpu->pc++);
    uint8_t hi = memory_read(cpu->mem, cpu->pc++);
    return ((hi << 8) | lo) + cpu->x;
}

static uint16_t absolute_y(nes_cpu_t *cpu) {
    uint8_t lo = memory_read(cpu->mem, cpu->pc++);
    uint8_t hi = memory_read(cpu->mem, cpu->pc++);
    return ((hi << 8) | lo) + cpu->y;
}

static uint16_t indirect_x(nes_cpu_t *cpu) {
    uint8_t zp_addr = (memory_read(cpu->mem, cpu->pc++) + cpu->x) & 0xFF;
    uint8_t lo = memory_read(cpu->mem, zp_addr);
    uint8_t hi = memory_read(cpu->mem, (zp_addr + 1) & 0xFF);
    return (hi << 8) | lo;
}

static uint16_t indirect_y(nes_cpu_t *cpu) {
    uint8_t zp_addr = memory_read(cpu->mem, cpu->pc++);
    uint8_t lo = memory_read(cpu->mem, zp_addr);
    uint8_t hi = memory_read(cpu->mem, (zp_addr + 1) & 0xFF);
    return ((hi << 8) | lo) + cpu->y;
}

// ==== Operações com Stack ====
#define STACK_BASE 0x0100

static void push(nes_cpu_t *cpu, uint8_t value)
{
    memory_write(cpu->mem, STACK_BASE + cpu->sp, value);
    cpu->sp--;
}

static uint8_t pull(nes_cpu_t *cpu)
{
    cpu->sp++;
    return memory_read(cpu->mem, STACK_BASE + cpu->sp);
}

// ==== Instruções básicas ====

// LDA - Load Accumulator
static void lda(nes_cpu_t *cpu, uint8_t value) {
    cpu->a = value;
    set_zn_flags(cpu, cpu->a);
}

// LDX - Load X
static void ldx(nes_cpu_t *cpu, uint8_t value) {
    cpu->x = value;
    set_zn_flags(cpu, cpu->x);
}

// LDY - Load Y
static void ldy(nes_cpu_t *cpu, uint8_t value) {
    cpu->y = value;
    set_zn_flags(cpu, cpu->y);
}

// STA - Store Accumulator
static void sta(nes_cpu_t *cpu, uint16_t addr) {
    memory_write(cpu->mem, addr, cpu->a);
}

// STX - Store X
static void stx(nes_cpu_t *cpu, uint16_t addr) {
    memory_write(cpu->mem, addr, cpu->x);
}

// STY - Store Y
static void sty(nes_cpu_t *cpu, uint16_t addr) {
    memory_write(cpu->mem, addr, cpu->y);
}

// ==== Operações aritméticas ====

// ADC - Add with Carry
static void adc(nes_cpu_t *cpu, uint8_t value) {
    uint16_t sum = cpu->a + value + (cpu->p & FLAG_C ? 1 : 0);

    // Carry flag
    if (sum > 0xFF) cpu->p |= FLAG_C;
    else cpu->p &= ~FLAG_C;

    // Overflow flag
    if (~(cpu->a ^ value) & (cpu->a ^ sum) & 0x80)
        cpu->p |= FLAG_V;
    else
        cpu->p &= ~FLAG_V;

    cpu->a = (uint8_t)sum;
    set_zn_flags(cpu, cpu->a);
}

// SBC - Subtract with Carry
static void sbc(nes_cpu_t *cpu, uint8_t value) {
    uint16_t diff = cpu->a - value - ((cpu->p & FLAG_C) ? 0 : 1);

    // Carry = 1 se não houve borrow
    if (diff < 0x100) cpu->p |= FLAG_C;
    else cpu->p &= ~FLAG_C;

    // Overflow
    if ((cpu->a ^ diff) & 0x80 && (cpu->a ^ value) & 0x80)
        cpu->p |= FLAG_V;
    else
        cpu->p &= ~FLAG_V;

    cpu->a = (uint8_t)diff;
    set_zn_flags(cpu, cpu->a);
}

// ==== Operações lógicas ====
static void and(nes_cpu_t *cpu, uint8_t value) {
    cpu->a &= value;
    set_zn_flags(cpu, cpu->a);
}

static void ora(nes_cpu_t *cpu, uint8_t value) {
    cpu->a |= value;
    set_zn_flags(cpu, cpu->a);
}

static void eor(nes_cpu_t *cpu, uint8_t value) {
    cpu->a ^= value;
    set_zn_flags(cpu, cpu->a);
}

// ==== Comparações ====
static void cmp(nes_cpu_t *cpu, uint8_t value) {
    uint16_t result = cpu->a - value;
    if (cpu->a >= value)
        cpu->p |= FLAG_C;
    else
        cpu->p &= ~FLAG_C;
    set_zn_flags(cpu, (uint8_t)result);
}

static void cpx(nes_cpu_t *cpu, uint8_t value) {
    uint16_t result = cpu->x - value;
    if (cpu->x >= value)
        cpu->p |= FLAG_C;
    else
        cpu->p &= ~FLAG_C;
    set_zn_flags(cpu, (uint8_t)result);
}

static void cpy(nes_cpu_t *cpu, uint8_t value) {
    uint16_t result = cpu->y - value;
    if (cpu->y >= value)
        cpu->p |= FLAG_C;
    else
        cpu->p &= ~FLAG_C;
    set_zn_flags(cpu, (uint8_t)result);
}

// ==== Incremento/Decremento ====
static void inx(nes_cpu_t *cpu) {
    cpu->x++;
    set_zn_flags(cpu, cpu->x);
}

static void dex(nes_cpu_t *cpu) {
    cpu->x--;
    set_zn_flags(cpu, cpu->x);
}

static void iny(nes_cpu_t *cpu) {
    cpu->y++;
    set_zn_flags(cpu, cpu->y);
}

static void dey(nes_cpu_t *cpu) {
    cpu->y--;
    set_zn_flags(cpu, cpu->y);
}

// ==== Controle de flags ====
static void sei(nes_cpu_t *cpu) { cpu->p |= FLAG_I; }
static void cli(nes_cpu_t *cpu) { cpu->p &= ~FLAG_I; }
static void clc(nes_cpu_t *cpu) { cpu->p &= ~FLAG_C; }
static void sec(nes_cpu_t *cpu) { cpu->p |= FLAG_C; }
static void sed(nes_cpu_t *cpu) { cpu->p |= FLAG_D; }
static void clv(nes_cpu_t *cpu) { cpu->p &= ~FLAG_V; }
static void nop(nes_cpu_t *cpu) { /* não faz nada */ }

// ==== Branches ====
static void beq(nes_cpu_t *cpu) {
    int8_t offset = (int8_t)immediate(cpu);
    if (cpu->p & FLAG_Z) {
        cpu->pc += offset;
    }
}

static void bne(nes_cpu_t *cpu) {
    int8_t offset = (int8_t)immediate(cpu);
    if (!(cpu->p & FLAG_Z)) {
        cpu->pc += offset;
    }
}

static void bmi(nes_cpu_t *cpu) {
    int8_t offset = (int8_t)immediate(cpu);
    if (cpu->p & FLAG_N) {
        cpu->pc += offset;
    }
}

static void bcc(nes_cpu_t *cpu) {
    int8_t offset = (int8_t)immediate(cpu);
    if (!(cpu->p & FLAG_C)) {
        cpu->pc += offset;
    }
}

static void bcs(nes_cpu_t *cpu) {
    int8_t offset = (int8_t)immediate(cpu);
    if (cpu->p & FLAG_C) {
        cpu->pc += offset;
    }
}

static void bvc(nes_cpu_t *cpu) {
    int8_t offset = (int8_t)immediate(cpu);
    if (!(cpu->p & FLAG_V)) {
        cpu->pc += offset;
    }
}

static void bvs(nes_cpu_t *cpu) {
    int8_t offset = (int8_t)immediate(cpu);
    if (cpu->p & FLAG_V) {
        cpu->pc += offset;
    }
}

// ==== Jumps e Calls ====
static void jmp_absolute(nes_cpu_t *cpu) {
    uint16_t addr = absolute(cpu);
    cpu->pc = addr;
}

static void jmp_indirect(nes_cpu_t *cpu) {
    uint16_t ptr = absolute(cpu);
    uint8_t lo = memory_read(cpu->mem, ptr);
    // BUG oficial 6502: indireto cruza página errado
    uint8_t hi = memory_read(cpu->mem, (ptr & 0xFF00) | ((ptr + 1) & 0x00FF));
    cpu->pc = (hi << 8) | lo;
}

static void jsr(nes_cpu_t *cpu) {
    uint16_t addr = absolute(cpu);
    uint16_t ret_addr = cpu->pc - 1;
    push(cpu, (ret_addr >> 8) & 0xFF);
    push(cpu, ret_addr & 0xFF);
    cpu->pc = addr;
}

static void rts(nes_cpu_t *cpu) {
    uint8_t lo = pull(cpu);
    uint8_t hi = pull(cpu);
    cpu->pc = ((hi << 8) | lo) + 1;
}

static void rti(nes_cpu_t *cpu) {
    cpu->p = pull(cpu);
    uint8_t lo = pull(cpu);
    uint8_t hi = pull(cpu);
    cpu->pc = (hi << 8) | lo;
}

static void brk(nes_cpu_t *cpu) {
    cpu->pc++;
    push(cpu, (cpu->pc >> 8) & 0xFF);
    push(cpu, cpu->pc & 0xFF);
    push(cpu, cpu->p | FLAG_B);
    cpu->p |= FLAG_I;
    
    uint8_t lo = memory_read(cpu->mem, 0xFFFE);
    uint8_t hi = memory_read(cpu->mem, 0xFFFF);
    cpu->pc = (hi << 8) | lo;
}

// ==== Stack operations ====
static void pha(nes_cpu_t *cpu) {
    push(cpu, cpu->a);
}

static void pla(nes_cpu_t *cpu) {
    cpu->a = pull(cpu);
    set_zn_flags(cpu, cpu->a);
}

static void php(nes_cpu_t *cpu) {
    push(cpu, cpu->p | FLAG_B | FLAG_U);
}

static void plp(nes_cpu_t *cpu) {
    cpu->p = pull(cpu);
    cpu->p |= FLAG_U;
}

// ==== Transfers ====
static void tax(nes_cpu_t *cpu) {
    cpu->x = cpu->a;
    set_zn_flags(cpu, cpu->x);
}

static void txa(nes_cpu_t *cpu) {
    cpu->a = cpu->x;
    set_zn_flags(cpu, cpu->a);
}

static void tay(nes_cpu_t *cpu) {
    cpu->y = cpu->a;
    set_zn_flags(cpu, cpu->y);
}

static void tya(nes_cpu_t *cpu) {
    cpu->a = cpu->y;
    set_zn_flags(cpu, cpu->a);
}

static void tsx(nes_cpu_t *cpu) {
    cpu->x = cpu->sp;
    set_zn_flags(cpu, cpu->x);
}

// ==== Função principal de execução ====
int cpu_step(nes_cpu_t *cpu, nes_memory_t *mem)
{
    uint8_t opcode = memory_read(cpu->mem, cpu->pc++);

    switch (opcode) {
        // LDA
        case 0xA9: lda(cpu, immediate(cpu)); break;
        case 0xA5: lda(cpu, memory_read(cpu->mem, zero_page(cpu))); break;
        case 0xAD: lda(cpu, memory_read(cpu->mem, absolute(cpu))); break;
        case 0xB5: lda(cpu, memory_read(cpu->mem, zero_page_x(cpu))); break;
        case 0xBD: lda(cpu, memory_read(cpu->mem, absolute_x(cpu))); break;
        case 0xB9: lda(cpu, memory_read(cpu->mem, absolute_y(cpu))); break;
        case 0xA1: lda(cpu, memory_read(cpu->mem, indirect_x(cpu))); break;
        case 0xB1: lda(cpu, memory_read(cpu->mem, indirect_y(cpu))); break;

        // LDX
        case 0xA2: ldx(cpu, immediate(cpu)); break;
        case 0xA6: ldx(cpu, memory_read(cpu->mem, zero_page(cpu))); break;
        case 0xB6: ldx(cpu, memory_read(cpu->mem, zero_page_y(cpu))); break;
        case 0xAE: ldx(cpu, memory_read(cpu->mem, absolute(cpu))); break;
        case 0xBE: ldx(cpu, memory_read(cpu->mem, absolute_y(cpu))); break;

        // LDY
        case 0xA0: ldy(cpu, immediate(cpu)); break;
        case 0xA4: ldy(cpu, memory_read(cpu->mem, zero_page(cpu))); break;
        case 0xB4: ldy(cpu, memory_read(cpu->mem, zero_page_x(cpu))); break;
        case 0xAC: ldy(cpu, memory_read(cpu->mem, absolute(cpu))); break;
        case 0xBC: ldy(cpu, memory_read(cpu->mem, absolute_x(cpu))); break;

        // STA
        case 0x85: sta(cpu, zero_page(cpu)); break;
        case 0x8D: sta(cpu, absolute(cpu)); break;
        case 0x95: sta(cpu, zero_page_x(cpu)); break;
        case 0x9D: sta(cpu, absolute_x(cpu)); break;
        case 0x99: sta(cpu, absolute_y(cpu)); break;
        case 0x81: sta(cpu, indirect_x(cpu)); break;
        case 0x91: sta(cpu, indirect_y(cpu)); break;

        // STX
        case 0x86: stx(cpu, zero_page(cpu)); break;
        case 0x96: stx(cpu, zero_page_y(cpu)); break;
        case 0x8E: stx(cpu, absolute(cpu)); break;

        // STY
        case 0x84: sty(cpu, zero_page(cpu)); break;
        case 0x94: sty(cpu, zero_page_x(cpu)); break;
        case 0x8C: sty(cpu, absolute(cpu)); break;

        // Branches
        case 0x10: // BPL
            if (!(cpu->p & FLAG_N)) {
                cpu->pc += (int8_t)immediate(cpu);
            } else {
                cpu->pc++;
            }
            break;
        case 0xF0: beq(cpu); break;
        case 0xD0: bne(cpu); break;
        case 0x30: bmi(cpu); break;
        case 0x90: bcc(cpu); break;
        case 0xB0: bcs(cpu); break;
        case 0x50: bvc(cpu); break;
        case 0x70: bvs(cpu); break;

        // Jumps
        case 0x4C: jmp_absolute(cpu); break;
        case 0x6C: jmp_indirect(cpu); break;
        case 0x20: jsr(cpu); break;
        case 0x60: rts(cpu); break;
        case 0x40: rti(cpu); break;

        // Flags
        case 0x18: clc(cpu); break;
        case 0x38: sec(cpu); break;
        case 0x58: cli(cpu); break;
        case 0x78: sei(cpu); break;
        case 0xD8: cpu->p &= ~FLAG_D; break; // CLD
        case 0xF8: sed(cpu); break;
        case 0xB8: clv(cpu); break;

        // Inc/Dec
        case 0xE8: inx(cpu); break;
        case 0xCA: dex(cpu); break;
        case 0xC8: iny(cpu); break;
        case 0x88: dey(cpu); break;

        // Transfers
        case 0xAA: tax(cpu); break;
        case 0x8A: txa(cpu); break;
        case 0xA8: tay(cpu); break;
        case 0x98: tya(cpu); break;
        case 0xBA: tsx(cpu); break;
        case 0x9A: cpu->sp = cpu->x; break; // TXS

        // Stack
        case 0x48: pha(cpu); break;
        case 0x68: pla(cpu); break;
        case 0x08: php(cpu); break;
        case 0x28: plp(cpu); break;

        // NOP
        case 0xEA: nop(cpu); break;

        // BRK
        case 0x00:
            printf("[CPU] BRK encontrado em PC=%04X\n", cpu->pc - 1);
            brk(cpu);
            return 0; // Sinaliza parada

        default:
            printf("[CPU] Opcode 0x%02X não implementado em PC=0x%04X\n",
                   opcode, cpu->pc - 1);
            break;
    }

    return 1; // Continua execução
}
