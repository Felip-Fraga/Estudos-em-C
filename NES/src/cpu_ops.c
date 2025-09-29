#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "memory.h"

// === Funções auxiliares ===

// Lê valor conforme modo de endereçamento
static uint8_t read_operand(nes_cpu_t *cpu, addr_mode_t mode, uint16_t *addr_out) {
    uint8_t value = 0;
    uint16_t lo, hi, addr;

    switch (mode) {
    case IMMEDIATE:
        value = memory_read(cpu->memory, cpu->pc++);
        break;

    case ZERO_PAGE:
        addr = memory_read(cpu->memory, cpu->pc++);
        if (addr_out) *addr_out = addr;
        value = memory_read(cpu->memory, addr);
        break;

    case ZERO_PAGE_X:
        addr = (memory_read(cpu->memory, cpu->pc++) + cpu->x) & 0xFF;
        if (addr_out) *addr_out = addr;
        value = memory_read(cpu->memory, addr);
        break;

    case ZERO_PAGE_Y:
        addr = (memory_read(cpu->memory, cpu->pc++) + cpu->y) & 0xFF;
        if (addr_out) *addr_out = addr;
        value = memory_read(cpu->memory, addr);
        break;

    case ABSOLUTE:
        lo = memory_read(cpu->memory, cpu->pc++);
        hi = memory_read(cpu->memory, cpu->pc++);
        addr = lo | (hi << 8);
        if (addr_out) *addr_out = addr;
        value = memory_read(cpu->memory, addr);
        break;

    case ABSOLUTE_X:
        lo = memory_read(cpu->memory, cpu->pc++);
        hi = memory_read(cpu->memory, cpu->pc++);
        addr = (lo | (hi << 8)) + cpu->x;
        if (addr_out) *addr_out = addr;
        value = memory_read(cpu->memory, addr);
        break;

    case ABSOLUTE_Y:
        lo = memory_read(cpu->memory, cpu->pc++);
        hi = memory_read(cpu->memory, cpu->pc++);
        addr = (lo | (hi << 8)) + cpu->y;
        if (addr_out) *addr_out = addr;
        value = memory_read(cpu->memory, addr);
        break;

    case INDIRECT:
        lo = memory_read(cpu->memory, cpu->pc++);
        hi = memory_read(cpu->memory, cpu->pc++);
        addr = lo | (hi << 8);
        {
            // Bug do 6502: se addr termina em 0xFF, o high byte vem de addr & 0xFF00
            uint16_t lo_ptr = memory_read(cpu->memory, addr);
            uint16_t hi_ptr = memory_read(cpu->memory, (addr & 0xFF00) | ((addr + 1) & 0xFF));
            addr = lo_ptr | (hi_ptr << 8);
        }
        if (addr_out) *addr_out = addr;
        value = memory_read(cpu->memory, addr);
        break;

    case INDIRECT_X:
        {
            uint8_t zp_addr = memory_read(cpu->memory, cpu->pc++);
            uint16_t ptr = (zp_addr + cpu->x) & 0xFF;
            uint16_t lo_ptr = memory_read(cpu->memory, ptr);
            uint16_t hi_ptr = memory_read(cpu->memory, (ptr + 1) & 0xFF);
            addr = lo_ptr | (hi_ptr << 8);
        }
        if (addr_out) *addr_out = addr;
        value = memory_read(cpu->memory, addr);
        break;

    case INDIRECT_Y:
        {
            uint8_t zp_addr = memory_read(cpu->memory, cpu->pc++);
            uint16_t lo_ptr = memory_read(cpu->memory, zp_addr);
            uint16_t hi_ptr = memory_read(cpu->memory, (zp_addr + 1) & 0xFF);
            addr = (lo_ptr | (hi_ptr << 8)) + cpu->y;
        }
        if (addr_out) *addr_out = addr;
        value = memory_read(cpu->memory, addr);
        break;

    case RELATIVE:
        {
            int8_t offset = (int8_t)memory_read(cpu->memory, cpu->pc++);
            addr = cpu->pc + offset;
            if (addr_out) *addr_out = addr;
            value = 0; // branches não leem valor
        }
        break;

    default:
        break;
    }

    return value;
}

// Atualiza flags Z/N
static void set_zn_flags(nes_cpu_t *cpu, uint8_t value) {
    cpu->status &= ~(FLAG_Z | FLAG_N);
    if (value == 0) cpu->status |= FLAG_Z;
    if (value & 0x80) cpu->status |= FLAG_N;
}

// Push para stack
static void push_stack(nes_cpu_t *cpu, uint8_t value) {
    memory_write(cpu->memory, 0x0100 + cpu->sp, value);
    cpu->sp--;
}

// Pop da stack
static uint8_t pop_stack(nes_cpu_t *cpu) {
    cpu->sp++;
    return memory_read(cpu->memory, 0x0100 + cpu->sp);
}

//
// ==== Implementações das instruções ====
//

// --- LOAD/STORE ---
void op_lda(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t value = read_operand(cpu, mode, NULL);
    cpu->a = value;
    set_zn_flags(cpu, cpu->a);
}

void op_ldx(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t value = read_operand(cpu, mode, NULL);
    cpu->x = value;
    set_zn_flags(cpu, cpu->x);
}

void op_ldy(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t value = read_operand(cpu, mode, NULL);
    cpu->y = value;
    set_zn_flags(cpu, cpu->y);
}

void op_sta(nes_cpu_t *cpu, addr_mode_t mode) {
    uint16_t addr = 0;
    read_operand(cpu, mode, &addr);
    memory_write(cpu->memory, addr, cpu->a);
}

void op_stx(nes_cpu_t *cpu, addr_mode_t mode) {
    uint16_t addr = 0;
    read_operand(cpu, mode, &addr);
    memory_write(cpu->memory, addr, cpu->x);
}

void op_sty(nes_cpu_t *cpu, addr_mode_t mode) {
    uint16_t addr = 0;
    read_operand(cpu, mode, &addr);
    memory_write(cpu->memory, addr, cpu->y);
}

// --- TRANSFER ---
void op_tax(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->x = cpu->a;
    set_zn_flags(cpu, cpu->x);
}

void op_tay(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->y = cpu->a;
    set_zn_flags(cpu, cpu->y);
}

void op_txa(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->a = cpu->x;
    set_zn_flags(cpu, cpu->a);
}

void op_tya(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->a = cpu->y;
    set_zn_flags(cpu, cpu->a);
}

void op_tsx(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->x = cpu->sp;
    set_zn_flags(cpu, cpu->x);
}

void op_txs(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->sp = cpu->x;
    // TXS não modifica flags
}

// --- ARITHMETIC ---
void op_adc(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t value = read_operand(cpu, mode, NULL);
    uint8_t carry = (cpu->status & FLAG_C) ? 1 : 0;
    uint16_t result = cpu->a + value + carry;
    
    // Overflow: se sinais iguais resultam em sinal diferente
    uint8_t overflow = ((cpu->a ^ result) & (value ^ result) & 0x80) != 0;
    
    cpu->status &= ~(FLAG_C | FLAG_Z | FLAG_V | FLAG_N);
    if (result > 0xFF) cpu->status |= FLAG_C;
    if (overflow) cpu->status |= FLAG_V;
    
    cpu->a = result & 0xFF;
    set_zn_flags(cpu, cpu->a);
}

void op_sbc(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t value = read_operand(cpu, mode, NULL);
    uint8_t carry = (cpu->status & FLAG_C) ? 0 : 1; // SBC usa carry invertido
    uint16_t result = cpu->a - value - carry;
    
    // Overflow: se sinais diferentes resultam em sinal igual ao subtraendo
    uint8_t overflow = ((cpu->a ^ value) & (cpu->a ^ result) & 0x80) != 0;
    
    cpu->status &= ~(FLAG_C | FLAG_Z | FLAG_V | FLAG_N);
    if (result <= 0xFF) cpu->status |= FLAG_C; // Carry clear se borrow
    if (overflow) cpu->status |= FLAG_V;
    
    cpu->a = result & 0xFF;
    set_zn_flags(cpu, cpu->a);
}

// --- LOGICAL ---
void op_and(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t value = read_operand(cpu, mode, NULL);
    cpu->a &= value;
    set_zn_flags(cpu, cpu->a);
}

void op_ora(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t value = read_operand(cpu, mode, NULL);
    cpu->a |= value;
    set_zn_flags(cpu, cpu->a);
}

void op_eor(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t value = read_operand(cpu, mode, NULL);
    cpu->a ^= value;
    set_zn_flags(cpu, cpu->a);
}

// --- COMPARE ---
void op_cmp(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t value = read_operand(cpu, mode, NULL);
    uint16_t result = cpu->a - value;
    
    cpu->status &= ~(FLAG_C | FLAG_Z | FLAG_N);
    if (cpu->a >= value) cpu->status |= FLAG_C;
    if (cpu->a == value) cpu->status |= FLAG_Z;
    if (result & 0x80) cpu->status |= FLAG_N;
}

void op_cpx(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t value = read_operand(cpu, mode, NULL);
    uint16_t result = cpu->x - value;
    
    cpu->status &= ~(FLAG_C | FLAG_Z | FLAG_N);
    if (cpu->x >= value) cpu->status |= FLAG_C;
    if (cpu->x == value) cpu->status |= FLAG_Z;
    if (result & 0x80) cpu->status |= FLAG_N;
}

void op_cpy(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t value = read_operand(cpu, mode, NULL);
    uint16_t result = cpu->y - value;
    
    cpu->status &= ~(FLAG_C | FLAG_Z | FLAG_N);
    if (cpu->y >= value) cpu->status |= FLAG_C;
    if (cpu->y == value) cpu->status |= FLAG_Z;
    if (result & 0x80) cpu->status |= FLAG_N;
}

// --- INCREMENT/DECREMENT ---
void op_inc(nes_cpu_t *cpu, addr_mode_t mode) {
    uint16_t addr = 0;
    uint8_t value = read_operand(cpu, mode, &addr);
    value++;
    memory_write(cpu->memory, addr, value);
    set_zn_flags(cpu, value);
}

void op_inx(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->x++;
    set_zn_flags(cpu, cpu->x);
}

void op_iny(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->y++;
    set_zn_flags(cpu, cpu->y);
}

void op_dec(nes_cpu_t *cpu, addr_mode_t mode) {
    uint16_t addr = 0;
    uint8_t value = read_operand(cpu, mode, &addr);
    value--;
    memory_write(cpu->memory, addr, value);
    set_zn_flags(cpu, value);
}

void op_dex(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->x--;
    set_zn_flags(cpu, cpu->x);
}

void op_dey(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->y--;
    set_zn_flags(cpu, cpu->y);
}

// --- SHIFTS ---
void op_asl(nes_cpu_t *cpu, addr_mode_t mode) {
    if (mode == ACCUMULATOR) {
        cpu->status = (cpu->status & ~FLAG_C) | ((cpu->a & 0x80) ? FLAG_C : 0);
        cpu->a <<= 1;
        set_zn_flags(cpu, cpu->a);
    } else {
        uint16_t addr = 0;
        uint8_t value = read_operand(cpu, mode, &addr);
        cpu->status = (cpu->status & ~FLAG_C) | ((value & 0x80) ? FLAG_C : 0);
        value <<= 1;
        memory_write(cpu->memory, addr, value);
        set_zn_flags(cpu, value);
    }
}

void op_lsr(nes_cpu_t *cpu, addr_mode_t mode) {
    if (mode == ACCUMULATOR) {
        cpu->status = (cpu->status & ~FLAG_C) | ((cpu->a & 0x01) ? FLAG_C : 0);
        cpu->a >>= 1;
        set_zn_flags(cpu, cpu->a);
    } else {
        uint16_t addr = 0;
        uint8_t value = read_operand(cpu, mode, &addr);
        cpu->status = (cpu->status & ~FLAG_C) | ((value & 0x01) ? FLAG_C : 0);
        value >>= 1;
        memory_write(cpu->memory, addr, value);
        set_zn_flags(cpu, value);
    }
}

void op_rol(nes_cpu_t *cpu, addr_mode_t mode) {
    if (mode == ACCUMULATOR) {
        uint8_t carry = (cpu->status & FLAG_C) ? 1 : 0;
        cpu->status = (cpu->status & ~FLAG_C) | ((cpu->a & 0x80) ? FLAG_C : 0);
        cpu->a = (cpu->a << 1) | carry;
        set_zn_flags(cpu, cpu->a);
    } else {
        uint16_t addr = 0;
        uint8_t value = read_operand(cpu, mode, &addr);
        uint8_t carry = (cpu->status & FLAG_C) ? 1 : 0;
        cpu->status = (cpu->status & ~FLAG_C) | ((value & 0x80) ? FLAG_C : 0);
        value = (value << 1) | carry;
        memory_write(cpu->memory, addr, value);
        set_zn_flags(cpu, value);
    }
}

void op_ror(nes_cpu_t *cpu, addr_mode_t mode) {
    if (mode == ACCUMULATOR) {
        uint8_t carry = (cpu->status & FLAG_C) ? 0x80 : 0;
        cpu->status = (cpu->status & ~FLAG_C) | ((cpu->a & 0x01) ? FLAG_C : 0);
        cpu->a = (cpu->a >> 1) | carry;
        set_zn_flags(cpu, cpu->a);
    } else {
        uint16_t addr = 0;
        uint8_t value = read_operand(cpu, mode, &addr);
        uint8_t carry = (cpu->status & FLAG_C) ? 0x80 : 0;
        cpu->status = (cpu->status & ~FLAG_C) | ((value & 0x01) ? FLAG_C : 0);
        value = (value >> 1) | carry;
        memory_write(cpu->memory, addr, value);
        set_zn_flags(cpu, value);
    }
}

// --- JUMPS ---
void op_jmp(nes_cpu_t *cpu, addr_mode_t mode) {
    uint16_t addr = 0;
    read_operand(cpu, mode, &addr);
    cpu->pc = addr;
}

void op_jsr(nes_cpu_t *cpu, addr_mode_t mode) {
    uint16_t addr = 0;
    read_operand(cpu, mode, &addr);
    
    // Push return address - 1 (RTS adiciona 1)
    uint16_t ret_addr = cpu->pc - 1;
    push_stack(cpu, (ret_addr >> 8) & 0xFF);
    push_stack(cpu, ret_addr & 0xFF);
    
    cpu->pc = addr;
}

void op_rts(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t lo = pop_stack(cpu);
    uint8_t hi = pop_stack(cpu);
    cpu->pc = (lo | (hi << 8)) + 1;
}

// --- BRANCHES ---
void op_bpl(nes_cpu_t *cpu, addr_mode_t mode) {
    if (!(cpu->status & FLAG_N)) {
        uint16_t addr = 0;
        read_operand(cpu, mode, &addr);
        cpu->pc = addr;
    } else {
        cpu->pc++; // pula o operando
    }
}

void op_bmi(nes_cpu_t *cpu, addr_mode_t mode) {
    if (cpu->status & FLAG_N) {
        uint16_t addr = 0;
        read_operand(cpu, mode, &addr);
        cpu->pc = addr;
    } else {
        cpu->pc++;
    }
}

void op_bvc(nes_cpu_t *cpu, addr_mode_t mode) {
    if (!(cpu->status & FLAG_V)) {
        uint16_t addr = 0;
        read_operand(cpu, mode, &addr);
        cpu->pc = addr;
    } else {
        cpu->pc++;
    }
}

void op_bvs(nes_cpu_t *cpu, addr_mode_t mode) {
    if (cpu->status & FLAG_V) {
        uint16_t addr = 0;
        read_operand(cpu, mode, &addr);
        cpu->pc = addr;
    } else {
        cpu->pc++;
    }
}

void op_bcc(nes_cpu_t *cpu, addr_mode_t mode) {
    if (!(cpu->status & FLAG_C)) {
        uint16_t addr = 0;
        read_operand(cpu, mode, &addr);
        cpu->pc = addr;
    } else {
        cpu->pc++;
    }
}

void op_bcs(nes_cpu_t *cpu, addr_mode_t mode) {
    if (cpu->status & FLAG_C) {
        uint16_t addr = 0;
        read_operand(cpu, mode, &addr);
        cpu->pc = addr;
    } else {
        cpu->pc++;
    }
}

void op_bne(nes_cpu_t *cpu, addr_mode_t mode) {
    if (!(cpu->status & FLAG_Z)) {
        uint16_t addr = 0;
        read_operand(cpu, mode, &addr);
        cpu->pc = addr;
    } else {
        cpu->pc++;
    }
}

void op_beq(nes_cpu_t *cpu, addr_mode_t mode) {
    if (cpu->status & FLAG_Z) {
        uint16_t addr = 0;
        read_operand(cpu, mode, &addr);
        cpu->pc = addr;
    } else {
        cpu->pc++;
    }
}

// --- FLAGS ---
void op_clc(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->status &= ~FLAG_C;
}

void op_sec(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->status |= FLAG_C;
}

void op_cli(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->status &= ~FLAG_I;
}

void op_sei(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->status |= FLAG_I;
}

void op_clv(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->status &= ~FLAG_V;
}

void op_cld(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->status &= ~FLAG_D;
}

void op_sed(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->status |= FLAG_D;
}

// --- STACK ---
void op_pha(nes_cpu_t *cpu, addr_mode_t mode) {
    push_stack(cpu, cpu->a);
}

void op_pla(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->a = pop_stack(cpu);
    set_zn_flags(cpu, cpu->a);
}

void op_php(nes_cpu_t *cpu, addr_mode_t mode) {
    push_stack(cpu, cpu->status | FLAG_B | FLAG_U);
}

void op_plp(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->status = pop_stack(cpu);
    cpu->status &= ~FLAG_B;  // Clear break flag
    cpu->status |= FLAG_U;   // Set unused flag
}

// --- INTERRUPTS ---
void op_brk(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->pc++; // BRK é 2 bytes
    
    // Push PC e status
    push_stack(cpu, (cpu->pc >> 8) & 0xFF);
    push_stack(cpu, cpu->pc & 0xFF);
    push_stack(cpu, cpu->status | FLAG_B | FLAG_U);
    
    // Set interrupt flag e jump para IRQ vector
    cpu->status |= FLAG_I;
    uint16_t lo = memory_read(cpu->memory, 0xFFFE);
    uint16_t hi = memory_read(cpu->memory, 0xFFFF);
    cpu->pc = lo | (hi << 8);
}

void op_rti(nes_cpu_t *cpu, addr_mode_t mode) {
    cpu->status = pop_stack(cpu);
    cpu->status &= ~FLAG_B;
    cpu->status |= FLAG_U;
    
    uint8_t lo = pop_stack(cpu);
    uint8_t hi = pop_stack(cpu);
    cpu->pc = lo | (hi << 8);
}

// --- BIT TEST ---
void op_bit(nes_cpu_t *cpu, addr_mode_t mode) {
    uint8_t value = read_operand(cpu, mode, NULL);
    uint8_t result = cpu->a & value;
    
    cpu->status &= ~(FLAG_Z | FLAG_V | FLAG_N);
    if (result == 0) cpu->status |= FLAG_Z;
    if (value & 0x40) cpu->status |= FLAG_V;
    if (value & 0x80) cpu->status |= FLAG_N;
}

// --- MISC ---
void op_nop(nes_cpu_t *cpu, addr_mode_t mode) {
    // Do nothing
}
