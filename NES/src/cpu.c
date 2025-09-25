#include "cpu.h"
#include <stdlib.h>
#include <stdio.h>

// ==== Inicialização/Reset/Liberação da CPU ====
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
// ==== ADC - Add with Carry ====
static void adc(nes_cpu_t *cpu, uint8_t value) {
    uint16_t sum = cpu->a + value + (cpu->p & FLAG_C ? 1 : 0);

    // Carry flag
    if (sum > 0xFF) cpu->p |= FLAG_C;
    else    cpu->p &= ~FLAG_C;

    // Overflow flag → quando sinal muda incorretamente
    if (~(cpu->a ^ value) & (cpu->a ^ sum) & 0x80)
    cpu->p |= FLAG_V;
    else
    cpu->p &= ~FLAG_V;

    cpu->a = (uint8_t)sum;
    set_zn_flags(cpu, cpu->a);
}

// ==== SBC - Subtract with Carry ====
// Implementado como A + (~M) + C
static void sbc(nes_cpu_t *cpu, uint8_t value) {
    uint16_t diff = cpu->a - value - ((cpu->p & FLAG_C) ? 0 : 1);

    // Carry = 1 se não houve borrow
    if (diff < 0x100) cpu->p |= FLAG_C;
    else    cpu->p &= ~FLAG_C;

    // Overflow
    if ((cpu->a ^ diff) & 0x80 && (cpu->a ^ value) & 0x80)
    cpu->p |= FLAG_V;
    else
    cpu->p &= ~FLAG_V;

    cpu->a = (uint8_t)diff;
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

// Zero Page, X
static uint16_t zero_page_x(nes_cpu_t *cpu) {
    uint8_t addr = (cpu_read(cpu->mem, cpu->pc++) + cpu->x) & 0xFF;
    return addr;
}

// Absolute, X
static uint16_t absolute_x(nes_cpu_t *cpu) {
    uint8_t lo = cpu_read(cpu->mem, cpu->pc++);
    uint8_t hi = cpu_read(cpu->mem, cpu->pc++);
    return ((hi << 8) | lo) + cpu->x;
}

// Absolute, Y
static uint16_t absolute_y(nes_cpu_t *cpu) {
    uint8_t lo = cpu_read(cpu->mem, cpu->pc++);
    uint8_t hi = cpu_read(cpu->mem, cpu->pc++);
    return ((hi << 8) | lo) + cpu->y;
}

// Indirect, X (pré-indexado)
static uint16_t indirect_x(nes_cpu_t *cpu) {
    uint8_t zp_addr = (cpu_read(cpu->mem, cpu->pc++) + cpu->x) & 0xFF;
    uint8_t lo = cpu_read(cpu->mem, zp_addr);
    uint8_t hi = cpu_read(cpu->mem, (zp_addr + 1) & 0xFF);
    return (hi << 8) | lo;
}

// Indirect, Y (pós-indexado)
static uint16_t indirect_y(nes_cpu_t *cpu) {
    uint8_t zp_addr = cpu_read(cpu->mem, cpu->pc++);
    uint8_t lo = cpu_read(cpu->mem, zp_addr);
    uint8_t hi = cpu_read(cpu->mem, (zp_addr + 1) & 0xFF);
    return ((hi << 8) | lo) + cpu->y;
}

// BCS - Branch if Carry Set
static void bcs(nes_cpu_t *cpu) {
    int8_t offset = (int8_t) immediate(cpu);
    if (cpu->p & FLAG_C) {
    cpu->pc += offset;
    }
}

// BMI - Branch if Minus (N == 1)
static void bmi(nes_cpu_t *cpu) {
    int8_t offset = (int8_t) immediate(cpu);
    if (cpu->p & FLAG_N) {
    cpu->pc += offset;
    }
}

// BCC - Branch if Carry Clear (C == 0)
static void bcc(nes_cpu_t *cpu) {
    int8_t offset = (int8_t) immediate(cpu);
    if (!(cpu->p & FLAG_C)) {
    cpu->pc += offset;
    }
}

// BVC - Branch if Overflow Clear (V == 0)
static void bvc(nes_cpu_t *cpu) {
    int8_t offset = (int8_t) immediate(cpu);
    if (!(cpu->p & FLAG_V)) {
    cpu->pc += offset;
    }
}

// BVS - Branch if Overflow Set (V == 1)
static void bvs(nes_cpu_t *cpu) {
    int8_t offset = (int8_t) immediate(cpu);
    if (cpu->p & FLAG_V) {
    cpu->pc += offset;
    }
}

// STX - Store X Register
static void stx(nes_cpu_t *cpu, uint16_t addr) {
    cpu_write(cpu->mem, addr, cpu->x);
}

// STY - Store Y Register
static void sty(nes_cpu_t *cpu, uint16_t addr) {
    cpu_write(cpu->mem, addr, cpu->y);
}

static uint16_t zero_page_y(nes_cpu_t *cpu) {
    uint8_t addr = (cpu_read(cpu->mem, cpu->pc++) + cpu->y) & 0xFF;
    return addr;
}

// ==== Operações com Stack ====
#define STACK_BASE 0x0100

static uint8_t pull(nes_cpu_t *cpu)
{
    cpu->sp++;
    return cpu_read(cpu->mem, STACK_BASE + cpu->sp);
}

// RTI
static void rti(nes_cpu_t *cpu) {
    // Pull status
    cpu->p = cpu_read(cpu->mem, 0x0100 + ++cpu->sp);
    // Pull PC
    uint8_t lo = cpu_read(cpu->mem, 0x0100 + ++cpu->sp);
    uint8_t hi = cpu_read(cpu->mem, 0x0100 + ++cpu->sp);
    cpu->pc = (hi << 8) | lo;
}

// BIT
static void bit(nes_cpu_t *cpu, uint8_t value) {
    uint8_t res = cpu->a & value;

    if (res == 0) cpu->p |= FLAG_Z;
    else    cpu->p &= ~FLAG_Z;

    if (value & 0x80) cpu->p |= FLAG_N; else cpu->p &= ~FLAG_N;
    if (value & 0x40) cpu->p |= FLAG_V; else cpu->p &= ~FLAG_V;
}

// ASL – Arithmetic Shift Left
static uint8_t do_asl(nes_cpu_t *cpu, uint8_t value) {
    if (value & 0x80) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C;
    uint8_t result = value << 1;
    set_zn_flags(cpu, result);
    return result;
}

// LSR – Logical Shift Right
static uint8_t do_lsr(nes_cpu_t *cpu, uint8_t value) {
    if (value & 0x01) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C;
    uint8_t result = value >> 1;
    set_zn_flags(cpu, result);
    return result;
}

// ROL – Rotate Left
static uint8_t do_rol(nes_cpu_t *cpu, uint8_t value) {
    uint8_t carry_in = (cpu->p & FLAG_C) ? 1 : 0;
    if (value & 0x80) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C;
    uint8_t result = (value << 1) | carry_in;
    set_zn_flags(cpu, result);
    return result;
}

// ROR – Rotate Right
static uint8_t do_ror(nes_cpu_t *cpu, uint8_t value) {
    uint8_t carry_in = (cpu->p & FLAG_C) ? 0x80 : 0;
    if (value & 0x01) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C;
    uint8_t result = (value >> 1) | carry_in;
    set_zn_flags(cpu, result);
    return result;
}

// PLA - Pull Accumulator
static void pla(nes_cpu_t *cpu) {
    cpu->a = pull(cpu);
    set_zn_flags(cpu, cpu->a);
}

static void push(nes_cpu_t *cpu, uint8_t value)
{
    cpu_write(cpu->mem, STACK_BASE + cpu->sp, value);
    cpu->sp--;
}


// PHP - Push Processor Status
static void php(nes_cpu_t *cpu) {
    // B flag e U flag setados quando empilha
    push(cpu, cpu->p | FLAG_B | FLAG_U);
}

// PLP - Pull Processor Status
static void plp(nes_cpu_t *cpu) {
    cpu->p = pull(cpu);
    cpu->p |= FLAG_U;  // U flag sempre 1
}

// TAX - Transfer A → X
static void tax(nes_cpu_t *cpu) {
    cpu->x = cpu->a;
    set_zn_flags(cpu, cpu->x);
}

// TXA - Transfer X → A
static void txa(nes_cpu_t *cpu) {
    cpu->a = cpu->x;
    set_zn_flags(cpu, cpu->a);
}

// TAY - Transfer A → Y
static void tay(nes_cpu_t *cpu) {
    cpu->y = cpu->a;
    set_zn_flags(cpu, cpu->y);
}

// TYA - Transfer Y → A
static void tya(nes_cpu_t *cpu) {
    cpu->a = cpu->y;
    set_zn_flags(cpu, cpu->a);
}

// TSX - Transfer SP → X
static void tsx(nes_cpu_t *cpu) {
    cpu->x = cpu->sp;
    set_zn_flags(cpu, cpu->x);
}

static uint16_t zero_page(nes_cpu_t *cpu)
{
    return cpu_read(cpu->mem, cpu->pc++);
}

// ==== Instruções implementadas ====

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

// ==== Execução ====

void cpu_step(nes_cpu_t *cpu)
{
    uint8_t opcode = cpu_read(cpu->mem, cpu->pc++);

    switch (opcode)
    {
    // ====
    // Loads e Stores
    // ====
    case 0xA9:
    lda(cpu, immediate(cpu));
    break; // LDA #imm
    case 0xA5:
    lda(cpu, cpu_read(cpu->mem, zero_page(cpu)));
    break; // LDA zp
    case 0xAD:
    lda(cpu, cpu_read(cpu->mem, absolute(cpu)));
    break; // LDA abs
    case 0x85:
    sta(cpu, zero_page(cpu));
    break; // STA zp
    case 0x8D:
    sta(cpu, absolute(cpu));
    break; // STA abs

    // ====
    // Bitwise
    // ====
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

    // ====
    // Controle / Flags
    // ====
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
    case 0xD8:
    cpu->p &= ~FLAG_D;
    break; // CLD (NES não usa Decimal, mas deve limpar a flag)
    case 0x00:
    printf("[CPU] BRK encontrado em PC=%04X, encerrando execução.\n", cpu->pc - 1);
    brk(cpu); return;
    break;

    // ====
    // Branch e Jump
    // ====
    case 0x4C:
    jmp_absolute(cpu);
    break; // JMP abs
    case 0x6C:
    jmp_indirect(cpu);
    break; // JMP ind
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

    // ====
    // Incremento / Decremento
    // ====
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

    // ====
    // Comparações
    // ====
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

    // ====
    // Transferências
    // ====
    case 0x9A:
    cpu->sp = cpu->x;
    break; // TXS (X → SP)

    // ====
    // LDA — Load Accumulator (novos modos)
    // ====
    case 0xB5: lda(cpu, cpu_read(cpu->mem, zero_page_x(cpu))); break; // LDA zp,X
    case 0xBD: lda(cpu, cpu_read(cpu->mem, absolute_x(cpu))); break;  // LDA abs,X
    case 0xB9: lda(cpu, cpu_read(cpu->mem, absolute_y(cpu))); break;  // LDA abs,Y
    case 0xA1: lda(cpu, cpu_read(cpu->mem, indirect_x(cpu))); break;  // LDA (ind,X)
    case 0xB1: lda(cpu, cpu_read(cpu->mem, indirect_y(cpu))); break;  // LDA (ind),Y

    // ====
    // Ilegais (NOP temporário)
    // ====
    case 0x07: // SLO zp
    case 0xD7: // DCP zp,X
    case 0xFF: // ISB abs,X
    case 0x02: // KIL
    case 0xFB: // ISC abs,Y
    nop(cpu);
    break;

    case 0x30: bmi(cpu); break; // BMI
    case 0x90: bcc(cpu); break; // BCC
    case 0xB0: bcs(cpu); break; // BCS
    case 0x50: bvc(cpu); break; // BVC
    case 0x70: bvs(cpu); break; // BVS

    // ====
    // STA — Store Accumulator (NOVOS modos)
    // ====
    case 0x95: sta(cpu, zero_page_x(cpu)); break;    // STA zp,X
    case 0x9D: sta(cpu, absolute_x(cpu)); break;    // STA abs,X
    case 0x99: sta(cpu, absolute_y(cpu)); break;    // STA abs,Y
    case 0x81: sta(cpu, indirect_x(cpu)); break;    // STA (ind,X)
    case 0x91: sta(cpu, indirect_y(cpu)); break;    // STA (ind),Y

    // ====
    // LDX — Load X
    // ====
    case 0xA2: ldx(cpu, immediate(cpu)); break;    // LDX #imm
    case 0xA6: ldx(cpu, cpu_read(cpu->mem, zero_page(cpu))); break;   // LDX zp
    case 0xB6: ldx(cpu, cpu_read(cpu->mem, zero_page_y(cpu))); break; // LDX zp,Y
    case 0xAE: ldx(cpu, cpu_read(cpu->mem, absolute(cpu))); break;    // LDX abs
    case 0xBE: ldx(cpu, cpu_read(cpu->mem, absolute_y(cpu))); break;  // LDX abs,Y

    // ====
    // LDY — Load Y
    // ====
    case 0xA0: ldy(cpu, immediate(cpu)); break;    // LDY #imm
    case 0xA4: ldy(cpu, cpu_read(cpu->mem, zero_page(cpu))); break;   // LDY zp
    case 0xB4: ldy(cpu, cpu_read(cpu->mem, zero_page_x(cpu))); break; // LDY zp,X
    case 0xAC: ldy(cpu, cpu_read(cpu->mem, absolute(cpu))); break;    // LDY abs
    case 0xBC: ldy(cpu, cpu_read(cpu->mem, absolute_x(cpu))); break;  // LDY abs,X

    // ====
    // STX — Store X
    // ====
    case 0x86: stx(cpu, zero_page(cpu)); break;    // STX zp
    case 0x96: stx(cpu, zero_page_y(cpu)); break;    // STX zp,Y
    case 0x8E: stx(cpu, absolute(cpu)); break;    // STX abs

    // ====
    // STY — Store Y
    // ====
    case 0x84: sty(cpu, zero_page(cpu)); break;    // STY zp
    case 0x94: sty(cpu, zero_page_x(cpu)); break;    // STY zp,X
    case 0x8C: sty(cpu, absolute(cpu)); break;    // STY abs

    // ====
    // NOP
    // ====
    case 0xEA: nop(cpu); break;  // NOP oficial

    // Vários NOPs ilegais (tratados como NOP simples)
    case 0x1A: case 0x3A: case 0x5A: case 0x7A:
    case 0xDA: case 0xFA: case 0x80: case 0x89:
    case 0x82: case 0xC2: case 0xE2:
    nop(cpu); break;

    // ====
    // JSR / RTS / RTI
    // ====
    case 0x20: jsr(cpu); break; // JSR abs
    case 0x60: rts(cpu); break; // RTS
    case 0x40: rti(cpu); break; // RTI

    // ====
    // BIT
    // ====
    case 0x24: bit(cpu, cpu_read(cpu->mem, zero_page(cpu))); break; // BIT zp
    case 0x2C: bit(cpu, cpu_read(cpu->mem, absolute(cpu))); break;  // BIT abs

    // ====
    // ADC — Add with Carry
    // ====
    case 0x69: adc(cpu, immediate(cpu)); break;    // ADC #imm
    case 0x65: adc(cpu, cpu_read(cpu->mem, zero_page(cpu))); break;   // ADC zp
    case 0x75: adc(cpu, cpu_read(cpu->mem, zero_page_x(cpu))); break; // ADC zp,X
    case 0x6D: adc(cpu, cpu_read(cpu->mem, absolute(cpu))); break;    // ADC abs
    case 0x7D: adc(cpu, cpu_read(cpu->mem, absolute_x(cpu))); break;  // ADC abs,X
    case 0x79: adc(cpu, cpu_read(cpu->mem, absolute_y(cpu))); break;  // ADC abs,Y
    case 0x61: adc(cpu, cpu_read(cpu->mem, indirect_x(cpu))); break;  // ADC (ind,X)
    case 0x71: adc(cpu, cpu_read(cpu->mem, indirect_y(cpu))); break;  // ADC (ind),Y

    // ====
    // SBC — Subtract with Carry
    // ====
    case 0xE9: sbc(cpu, immediate(cpu)); break;    // SBC #imm
    case 0xE5: sbc(cpu, cpu_read(cpu->mem, zero_page(cpu))); break;   // SBC zp
    case 0xF5: sbc(cpu, cpu_read(cpu->mem, zero_page_x(cpu))); break; // SBC zp,X
    case 0xED: sbc(cpu, cpu_read(cpu->mem, absolute(cpu))); break;    // SBC abs
    case 0xFD: sbc(cpu, cpu_read(cpu->mem, absolute_x(cpu))); break;  // SBC abs,X
    case 0xF9: sbc(cpu, cpu_read(cpu->mem, absolute_y(cpu))); break;  // SBC abs,Y
    case 0xE1: sbc(cpu, cpu_read(cpu->mem, indirect_x(cpu))); break;  // SBC (ind,X)
    case 0xF1: sbc(cpu, cpu_read(cpu->mem, indirect_y(cpu))); break;  // SBC (ind),Y

    // ==== Shifts e Rotates ====
    // ASL
    case 0x0A: cpu->a = do_asl(cpu, cpu->a); break;    // ASL A
    case 0x06: { uint16_t addr = zero_page(cpu); cpu_write(cpu->mem, addr, do_asl(cpu, cpu_read(cpu->mem, addr))); } break; // ASL zp
    case 0x16: { uint16_t addr = zero_page_x(cpu); cpu_write(cpu->mem, addr, do_asl(cpu, cpu_read(cpu->mem, addr))); } break; // ASL zp,X
    case 0x0E: { uint16_t addr = absolute(cpu); cpu_write(cpu->mem, addr, do_asl(cpu, cpu_read(cpu->mem, addr))); } break; // ASL abs
    case 0x1E: { uint16_t addr = absolute_x(cpu); cpu_write(cpu->mem, addr, do_asl(cpu, cpu_read(cpu->mem, addr))); } break; // ASL abs,X

    // LSR
    case 0x4A: cpu->a = do_lsr(cpu, cpu->a); break;
    case 0x46: { uint16_t addr = zero_page(cpu); cpu_write(cpu->mem, addr, do_lsr(cpu, cpu_read(cpu->mem, addr))); } break;
    case 0x56: { uint16_t addr = zero_page_x(cpu); cpu_write(cpu->mem, addr, do_lsr(cpu, cpu_read(cpu->mem, addr))); } break;
    case 0x4E: { uint16_t addr = absolute(cpu); cpu_write(cpu->mem, addr, do_lsr(cpu, cpu_read(cpu->mem, addr))); } break;
    case 0x5E: { uint16_t addr = absolute_x(cpu); cpu_write(cpu->mem, addr, do_lsr(cpu, cpu_read(cpu->mem, addr))); } break;

    // ROL
    case 0x2A: cpu->a = do_rol(cpu, cpu->a); break;
    case 0x26: { uint16_t addr = zero_page(cpu); cpu_write(cpu->mem, addr, do_rol(cpu, cpu_read(cpu->mem, addr))); } break;
    case 0x36: { uint16_t addr = zero_page_x(cpu); cpu_write(cpu->mem, addr, do_rol(cpu, cpu_read(cpu->mem, addr))); } break;
    case 0x2E: { uint16_t addr = absolute(cpu); cpu_write(cpu->mem, addr, do_rol(cpu, cpu_read(cpu->mem, addr))); } break;
    case 0x3E: { uint16_t addr = absolute_x(cpu); cpu_write(cpu->mem, addr, do_rol(cpu, cpu_read(cpu->mem, addr))); } break;

    // ROR
    case 0x6A: cpu->a = do_ror(cpu, cpu->a); break;
    case 0x66: { uint16_t addr = zero_page(cpu); cpu_write(cpu->mem, addr, do_ror(cpu, cpu_read(cpu->mem, addr))); } break;
    case 0x76: { uint16_t addr = zero_page_x(cpu); cpu_write(cpu->mem, addr, do_ror(cpu, cpu_read(cpu->mem, addr))); } break;
    case 0x6E: { uint16_t addr = absolute(cpu); cpu_write(cpu->mem, addr, do_ror(cpu, cpu_read(cpu->mem, addr))); } break;
    case 0x7E: { uint16_t addr = absolute_x(cpu); cpu_write(cpu->mem, addr, do_ror(cpu, cpu_read(cpu->mem, addr))); } break;

    // CMP adicionais
    case 0xD5: cmp(cpu, cpu_read(cpu->mem, zero_page_x(cpu))); break; // CMP zp,X
    case 0xDD: cmp(cpu, cpu_read(cpu->mem, absolute_x(cpu))); break;  // CMP abs,X
    case 0xD9: cmp(cpu, cpu_read(cpu->mem, absolute_y(cpu))); break;  // CMP abs,Y
    case 0xC1: cmp(cpu, cpu_read(cpu->mem, indirect_x(cpu))); break;  // CMP (ind,X)
    case 0xD1: cmp(cpu, cpu_read(cpu->mem, indirect_y(cpu))); break;  // CMP (ind),Y

    // CPX adicionais
    case 0xE4: cpx(cpu, cpu_read(cpu->mem, zero_page(cpu))); break;   // CPX zp
    case 0xEC: cpx(cpu, cpu_read(cpu->mem, absolute(cpu))); break;    // CPX abs

    // CPY adicionais
    case 0xC4: cpy(cpu, cpu_read(cpu->mem, zero_page(cpu))); break;   // CPY zp
    case 0xCC: cpy(cpu, cpu_read(cpu->mem, absolute(cpu))); break;    // CPY abs

    // ==== Stack ====
    case 0x48: pha(cpu); break; // PHA
    case 0x68: pla(cpu); break; // PLA
    case 0x08: php(cpu); break; // PHP
    case 0x28: plp(cpu); break; // PLP

    // ==== Transferências ====
    case 0xAA: tax(cpu); break; // TAX
    case 0x8A: txa(cpu); break; // TXA
    case 0xA8: tay(cpu); break; // TAY
    case 0x98: tya(cpu); break; // TYA
    case 0xBA: tsx(cpu); break; // TSX


    // ==== INC/DEC Memory ====
    case 0xE6: inc(cpu, zero_page(cpu)); break;    // INC zp
    case 0xF6: inc(cpu, zero_page_x(cpu)); break;  // INC zp,X
    case 0xEE: inc(cpu, absolute(cpu)); break;     // INC abs
    case 0xFE: inc(cpu, absolute_x(cpu)); break;   // INC abs,X

    case 0xC6: dec(cpu, zero_page(cpu)); break;    // DEC zp
    case 0xD6: dec(cpu, zero_page_x(cpu)); break;  // DEC zp,X
    case 0xCE: dec(cpu, absolute(cpu)); break;     // DEC abs
    case 0xDE: dec(cpu, absolute_x(cpu)); break;   // DEC abs,X

    // ==== Flags adicionais ====
    case 0xF8: sed(cpu); break; // SED
    case 0xB8: clv(cpu); break; // CLV
    // ====
    // Default (não implementados)
    // ====
    default:
    printf("[CPU] Opcode 0x%02X não implementado em PC=0x%04X\n",
    opcode, cpu->pc - 1);
    break;
    }

    printf("[CPU] PC=%04X A=%02X X=%02X Y=%02X SP=%02X P=%02X\n",
    cpu->pc, cpu->a, cpu->x, cpu->y, cpu->sp, cpu->p);
}

// ==== FUNÇÕES ADICIONAIS FALTANTES ====

// PHA - Push Accumulator
static void pha(nes_cpu_t *cpu) {
    push(cpu, cpu->a);
}

// INC - Increment Memory
static void inc(nes_cpu_t *cpu, uint16_t addr) {
    uint8_t value = cpu_read(cpu->mem, addr) + 1;
    cpu_write(cpu->mem, addr, value);
    set_zn_flags(cpu, value);
}

// DEC - Decrement Memory
static void dec(nes_cpu_t *cpu, uint16_t addr) {
    uint8_t value = cpu_read(cpu->mem, addr) - 1;
    cpu_write(cpu->mem, addr, value);
    set_zn_flags(cpu, value);
}

// SED - Set Decimal Flag
static void sed(nes_cpu_t *cpu) {
    cpu->p |= FLAG_D;
}

// CLV - Clear Overflow Flag
static void clv(nes_cpu_t *cpu) {
    cpu->p &= ~FLAG_V;
}

// BRK - Software Interrupt
static void brk(nes_cpu_t *cpu) {
    cpu->pc++; // BRK é 2 bytes
    push(cpu, (cpu->pc >> 8) & 0xFF);
    push(cpu, cpu->pc & 0xFF);
    push(cpu, cpu->p | FLAG_B);
    cpu->p |= FLAG_I;
    
    // Vetor IRQ/BRK em $FFFE-$FFFF
    uint8_t lo = cpu_read(cpu->mem, 0xFFFE);
    uint8_t hi = cpu_read(cpu->mem, 0xFFFF);
    cpu->pc = (hi << 8) | lo;
}
