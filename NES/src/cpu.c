#include "cpu.h"
#include <stdlib.h>
#include <stdio.h>

// ===== Inicialização/Reset/Liberação da CPU =====
nes_cpu_t *cpu_init(nes_memory_t *mem)
{
    nes_cpu_t *cpu = malloc(sizeof(nes_cpu_t));
    if (!cpu)
        return NULL;

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
    uint16_t lo = cpu_read(cpu->mem, 0xFFFC);
    uint16_t hi = cpu_read(cpu->mem, 0xFFFD);
    cpu->pc = (hi << 8) | lo;

    printf("[CPU] Reset concluído. PC inicial = 0x%04X\n", cpu->pc);
}

void cpu_free(nes_cpu_t *cpu)
{
    if (cpu)
        free(cpu);
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

// Função auxiliar para ADC/SBC → define flags Z/N/C/V
static void set_adc_flags(nes_cpu_t *cpu, uint8_t a, uint8_t b, uint16_t result)
{
    // Carry
    if (result > 0xFF)
        cpu->p |= FLAG_C;
    else
        cpu->p &= ~FLAG_C;

    // Overflow (ocorre se sinal do resultado não cabe em 8 bits com mesmos sinais)
    if (((a ^ b) & 0x80) == 0 && ((a ^ result) & 0x80) != 0)
        cpu->p |= FLAG_V;
    else
        cpu->p &= ~FLAG_V;

    // Zero/Negative
    set_zn_flags(cpu, (uint8_t)result);
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

// STA  (Store Accumulator)
static void sta(nes_cpu_t *cpu, uint16_t addr) {
    cpu_write(cpu->mem, addr, cpu->a);
}
// ADC - Add with Carry
static void adc(nes_cpu_t *cpu, uint8_t value)
{
    uint16_t sum = cpu->a + value + (cpu->p & FLAG_C ? 1 : 0);
    set_adc_flags(cpu, cpu->a, value, sum);
    cpu->a = (uint8_t)(sum & 0xFF);
}

// SBC - Subtract with Carry
static void sbc(nes_cpu_t *cpu, uint8_t value)
{
    // SBC = A + (~value) + Carry
    uint16_t diff = cpu->a - value - ((cpu->p & FLAG_C) ? 0 : 1);
    // Atualiza Carry (setado se não houve “borrow”)
    if (cpu->a >= (uint8_t)(value + ((cpu->p & FLAG_C) ? 0 : 1)))
        cpu->p |= FLAG_C;
    else
        cpu->p &= ~FLAG_C;

    // Overflow
    if (((cpu->a ^ value) & 0x80) && ((cpu->a ^ diff) & 0x80))
        cpu->p |= FLAG_V;
    else
        cpu->p &= ~FLAG_V;

    cpu->a = (uint8_t)(diff & 0xFF);
    set_zn_flags(cpu, cpu->a);
}

// AND
static void and(nes_cpu_t *cpu, uint8_t value)
{
    cpu->a &= value;
    set_zn_flags(cpu, cpu->a);
}

// ORA
static void ora(nes_cpu_t *cpu, uint8_t value)
{
    cpu->a |= value;
    set_zn_flags(cpu, cpu->a);
}

// EOR
static void eor(nes_cpu_t *cpu, uint8_t value)
{
    cpu->a ^= value;
    set_zn_flags(cpu, cpu->a);
}

// --- Controle/Flags ---
static void sei(nes_cpu_t *cpu) { cpu->p |= FLAG_I; }
static void cli(nes_cpu_t *cpu) { cpu->p &= ~FLAG_I; }
static void clc(nes_cpu_t *cpu) { cpu->p &= ~FLAG_C; }
static void sec(nes_cpu_t *cpu) { cpu->p |= FLAG_C; }
static void nop(nes_cpu_t *cpu) { /* não faz nada */ }

// --- Incremento/Decremento ---
static void inx(nes_cpu_t *cpu)
{
    cpu->x++;
    set_zn_flags(cpu, cpu->x);
}
static void dex(nes_cpu_t *cpu)
{
    cpu->x--;
    set_zn_flags(cpu, cpu->x);
}
static void iny(nes_cpu_t *cpu)
{
    cpu->y++;
    set_zn_flags(cpu, cpu->y);
}
static void dey(nes_cpu_t *cpu)
{
    cpu->y--;
    set_zn_flags(cpu, cpu->y);
}

// --- Comparações (alteram Z, N, C) ---
static void cmp(nes_cpu_t *cpu, uint8_t value)
{
    uint16_t result = cpu->a - value;
    if (cpu->a >= value)
        cpu->p |= FLAG_C;
    else
        cpu->p &= ~FLAG_C;
    set_zn_flags(cpu, (uint8_t)result);
}

static void cpx(nes_cpu_t *cpu, uint8_t value)
{
    uint16_t result = cpu->x - value;
    if (cpu->x >= value)
        cpu->p |= FLAG_C;
    else
        cpu->p &= ~FLAG_C;
    set_zn_flags(cpu, (uint8_t)result);
}

static void cpy(nes_cpu_t *cpu, uint8_t value)
{
    uint16_t result = cpu->y - value;
    if (cpu->y >= value)
        cpu->p |= FLAG_C;
    else
        cpu->p &= ~FLAG_C;
    set_zn_flags(cpu, (uint8_t)result);
}

static uint8_t immediate(nes_cpu_t *cpu)
{
    return cpu_read(cpu->mem, cpu->pc++);
}

static uint16_t absolute(nes_cpu_t *cpu)
{
    uint8_t lo = cpu_read(cpu->mem, cpu->pc++);
    uint8_t hi = cpu_read(cpu->mem, cpu->pc++);
    return (hi << 8) | lo;
}

static uint16_t zero_page(nes_cpu_t *cpu)
{
    return cpu_read(cpu->mem, cpu->pc++);
}

// ===== Operações com Stack =====
#define STACK_BASE 0x0100

static void push(nes_cpu_t *cpu, uint8_t value)
{
    cpu_write(cpu->mem, STACK_BASE + cpu->sp, value);
    cpu->sp--;
}

static uint8_t pull(nes_cpu_t *cpu)
{
    cpu->sp++;
    return cpu_read(cpu->mem, STACK_BASE + cpu->sp);
}

// ===== Instruções implementadas =====

// LDA
static void lda(nes_cpu_t *cpu, uint8_t value)
{
    cpu->a = value;
    set_zn_flags(cpu, cpu->a);
}

// JMP absoluto
static void jmp_absolute(nes_cpu_t *cpu)
{
    uint16_t addr = absolute(cpu);
    cpu->pc = addr;
}

// JMP indireto
static void jmp_indirect(nes_cpu_t *cpu)
{
    uint16_t ptr = absolute(cpu);
    uint8_t lo = cpu_read(cpu->mem, ptr);
    // BUG oficial 6502: indireto lê cruza página errado (emulamos também)
    uint8_t hi = cpu_read(cpu->mem, (ptr & 0xFF00) | ((ptr + 1) & 0x00FF));
    cpu->pc = (hi << 8) | lo;
}

// JSR
static void jsr(nes_cpu_t *cpu)
{
    uint16_t addr = absolute(cpu);
    uint16_t ret_addr = cpu->pc - 1; // endereço de retorno
    push(cpu, (ret_addr >> 8) & 0xFF);
    push(cpu, ret_addr & 0xFF);
    cpu->pc = addr;
}

// RTS
static void rts(nes_cpu_t *cpu)
{
    uint8_t lo = pull(cpu);
    uint8_t hi = pull(cpu);
    cpu->pc = ((hi << 8) | lo) + 1;
}

// BEQ
static void beq(nes_cpu_t *cpu)
{
    int8_t offset = (int8_t)immediate(cpu);
    if (cpu->p & FLAG_Z)
    {
        cpu->pc += offset;
    }
}

// BNE
static void bne(nes_cpu_t *cpu)
{
    int8_t offset = (int8_t)immediate(cpu);
    if (!(cpu->p & FLAG_Z))
    {
        cpu->pc += offset;
    }
}

// ===== Execução =====

void cpu_step(nes_cpu_t *cpu)
{
    uint8_t opcode = cpu_read(cpu->mem, cpu->pc++);

    switch (opcode)
    {
    // ===============================
    // Loads e Stores
    // ===============================
    case 0xA9:
        lda(cpu, immediate(cpu));
        break; // LDA #imm
    case 0xA5:
        lda(cpu, cpu_read(cpu->mem, zero_page(cpu)));
        break; // LDA zp
    case 0xAD:
        lda(cpu, cpu_read(cpu->mem, absolute(cpu)));
        break; // LDA abs

    case 0xA2:
        ldx(cpu, immediate(cpu));
        break; // LDX #imm
    case 0xA0:
        ldy(cpu, immediate(cpu));
        break; // LDY #imm

    case 0x85:
        sta(cpu, zero_page(cpu));
        break; // STA zp
    case 0x8D:
        sta(cpu, absolute(cpu));
        break; // STA abs

    // ===============================
    // Aritmética
    // ===============================
    case 0x69:
        adc(cpu, immediate(cpu));
        break; // ADC #imm
    case 0x65:
        adc(cpu, cpu_read(cpu->mem, zero_page(cpu)));
        break; // ADC zp
    case 0x6D:
        adc(cpu, cpu_read(cpu->mem, absolute(cpu)));
        break; // ADC abs

    case 0xE9:
        sbc(cpu, immediate(cpu));
        break; // SBC #imm
    case 0xE5:
        sbc(cpu, cpu_read(cpu->mem, zero_page(cpu)));
        break; // SBC zp
    case 0xED:
        sbc(cpu, cpu_read(cpu->mem, absolute(cpu)));
        break; // SBC abs

    // ===============================
    // Bitwise
    // ===============================
    case 0x29:
        and(cpu, immediate(cpu));
        break; // AND #imm
    case 0x25:
        and(cpu, cpu_read(cpu->mem, zero_page(cpu)));
        break; // AND zp
    case 0x2D:
        and(cpu, cpu_read(cpu->mem, absolute(cpu)));
        break; // AND abs

    case 0x09:
        ora(cpu, immediate(cpu));
        break; // ORA #imm
    case 0x05:
        ora(cpu, cpu_read(cpu->mem, zero_page(cpu)));
        break; // ORA zp
    case 0x0D:
        ora(cpu, cpu_read(cpu->mem, absolute(cpu)));
        break; // ORA abs

    case 0x49:
        eor(cpu, immediate(cpu));
        break; // EOR #imm
    case 0x45:
        eor(cpu, cpu_read(cpu->mem, zero_page(cpu)));
        break; // EOR zp
    case 0x4D:
        eor(cpu, cpu_read(cpu->mem, absolute(cpu)));
        break; // EOR abs

    // ===============================
    // Controle / Flags
    // ===============================
    case 0x78:
        sei(cpu);
        break; // SEI
    case 0x58:
        cli(cpu);
        break; // CLI
    case 0x18:
        clc(cpu);
        break; // CLC
    case 0x38:
        sec(cpu);
        break; // SEC
    case 0xEA:
        nop(cpu);
        break; // NOP
    case 0xD8:
        cpu->p &= ~FLAG_D;
        break; // CLD (NES não usa Decimal, mas deve limpar a flag)
    case 0x00:
        printf("[CPU] BRK encontrado em PC=%04X, encerrando execução.\n", cpu->pc - 1);
        exit(0);
        break;

    // ===============================
    // Branch e Jump
    // ===============================
    case 0x4C:
        jmp_absolute(cpu);
        break; // JMP abs
    case 0x6C:
        jmp_indirect(cpu);
        break; // JMP ind

    case 0x20:
        jsr(cpu);
        break; // JSR abs
    case 0x60:
        rts(cpu);
        break; // RTS

    case 0xF0:
        beq(cpu);
        break; // BEQ
    case 0xD0:
        bne(cpu);
        break; // BNE
    case 0x10:
        if (!(cpu->p & FLAG_N))
        {
            cpu->pc += (int8_t)immediate(cpu);
        }
        else
        {
            cpu->pc++;
        }
        break; // BPL

    // ===============================
    // Incremento / Decremento
    // ===============================
    case 0xE8:
        inx(cpu);
        break; // INX
    case 0xCA:
        dex(cpu);
        break; // DEX
    case 0xC8:
        iny(cpu);
        break; // INY
    case 0x88:
        dey(cpu);
        break; // DEY

    // ===============================
    // Comparações
    // ===============================
    case 0xC9:
        cmp(cpu, immediate(cpu));
        break; // CMP #imm
    case 0xC5:
        cmp(cpu, cpu_read(cpu->mem, zero_page(cpu)));
        break; // CMP zp
    case 0xCD:
        cmp(cpu, cpu_read(cpu->mem, absolute(cpu)));
        break; // CMP abs

    case 0xE0:
        cpx(cpu, immediate(cpu));
        break; // CPX #imm
    case 0xC0:
        cpy(cpu, immediate(cpu));
        break; // CPY #imm

    // ===============================
    // Transferências
    // ===============================
    case 0x9A:
        cpu->sp = cpu->x;
        break; // TXS (X → SP)

    // ===============================
    // Default (não implementados)
    // ===============================
    default:
        printf("[CPU] Opcode 0x%02X não implementado em PC=0x%04X\n",
               opcode, cpu->pc - 1);
        break;
    }

    printf("[CPU] PC=%04X A=%02X X=%02X Y=%02X SP=%02X P=%02X\n",
           cpu->pc, cpu->a, cpu->x, cpu->y, cpu->sp, cpu->p);
}