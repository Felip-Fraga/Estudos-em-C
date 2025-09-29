#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declarations das funções de operação
void op_lda(nes_cpu_t *cpu, addr_mode_t mode);
void op_ldx(nes_cpu_t *cpu, addr_mode_t mode);
void op_ldy(nes_cpu_t *cpu, addr_mode_t mode);
void op_sta(nes_cpu_t *cpu, addr_mode_t mode);
void op_stx(nes_cpu_t *cpu, addr_mode_t mode);
void op_sty(nes_cpu_t *cpu, addr_mode_t mode);
void op_tax(nes_cpu_t *cpu, addr_mode_t mode);
void op_tay(nes_cpu_t *cpu, addr_mode_t mode);
void op_txa(nes_cpu_t *cpu, addr_mode_t mode);
void op_tya(nes_cpu_t *cpu, addr_mode_t mode);
void op_tsx(nes_cpu_t *cpu, addr_mode_t mode);
void op_txs(nes_cpu_t *cpu, addr_mode_t mode);
void op_jmp(nes_cpu_t *cpu, addr_mode_t mode);
void op_jsr(nes_cpu_t *cpu, addr_mode_t mode);
void op_rts(nes_cpu_t *cpu, addr_mode_t mode);
void op_brk(nes_cpu_t *cpu, addr_mode_t mode);
void op_rti(nes_cpu_t *cpu, addr_mode_t mode);
void op_bpl(nes_cpu_t *cpu, addr_mode_t mode);
void op_bmi(nes_cpu_t *cpu, addr_mode_t mode);
void op_bvc(nes_cpu_t *cpu, addr_mode_t mode);
void op_bvs(nes_cpu_t *cpu, addr_mode_t mode);
void op_bcc(nes_cpu_t *cpu, addr_mode_t mode);
void op_bcs(nes_cpu_t *cpu, addr_mode_t mode);
void op_bne(nes_cpu_t *cpu, addr_mode_t mode);
void op_beq(nes_cpu_t *cpu, addr_mode_t mode);
void op_cld(nes_cpu_t *cpu, addr_mode_t mode);
void op_cli(nes_cpu_t *cpu, addr_mode_t mode);
void op_clv(nes_cpu_t *cpu, addr_mode_t mode);
void op_clc(nes_cpu_t *cpu, addr_mode_t mode);
void op_sed(nes_cpu_t *cpu, addr_mode_t mode);
void op_sei(nes_cpu_t *cpu, addr_mode_t mode);
void op_sec(nes_cpu_t *cpu, addr_mode_t mode);
void op_nop(nes_cpu_t *cpu, addr_mode_t mode);
void op_adc(nes_cpu_t *cpu, addr_mode_t mode);
void op_sbc(nes_cpu_t *cpu, addr_mode_t mode);
void op_and(nes_cpu_t *cpu, addr_mode_t mode);
void op_ora(nes_cpu_t *cpu, addr_mode_t mode);
void op_eor(nes_cpu_t *cpu, addr_mode_t mode);
void op_cmp(nes_cpu_t *cpu, addr_mode_t mode);
void op_cpx(nes_cpu_t *cpu, addr_mode_t mode);
void op_cpy(nes_cpu_t *cpu, addr_mode_t mode);
void op_inc(nes_cpu_t *cpu, addr_mode_t mode);
void op_inx(nes_cpu_t *cpu, addr_mode_t mode);
void op_iny(nes_cpu_t *cpu, addr_mode_t mode);
void op_dec(nes_cpu_t *cpu, addr_mode_t mode);
void op_dex(nes_cpu_t *cpu, addr_mode_t mode);
void op_dey(nes_cpu_t *cpu, addr_mode_t mode);
void op_asl(nes_cpu_t *cpu, addr_mode_t mode);
void op_lsr(nes_cpu_t *cpu, addr_mode_t mode);
void op_rol(nes_cpu_t *cpu, addr_mode_t mode);
void op_ror(nes_cpu_t *cpu, addr_mode_t mode);
void op_bit(nes_cpu_t *cpu, addr_mode_t mode);
void op_pha(nes_cpu_t *cpu, addr_mode_t mode);
void op_pla(nes_cpu_t *cpu, addr_mode_t mode);
void op_php(nes_cpu_t *cpu, addr_mode_t mode);
void op_plp(nes_cpu_t *cpu, addr_mode_t mode);

// Tabela de instruções (256 entradas)
instruction_t instructions[256];

// Função para inicializar a tabela de instruções
void init_instructions(void) {
    // Inicializa todas as instruções como não implementadas
    for (int i = 0; i < 256; i++) {
        instructions[i].name = "???";
        instructions[i].mode = IMPLIED;
        instructions[i].bytes = 1;
        instructions[i].cycles = 2;
        instructions[i].execute = NULL;
    }

    // === LOAD/STORE ===
    instructions[0xA9] = (instruction_t){ "LDA", IMMEDIATE,   2, 2, op_lda };
    instructions[0xA5] = (instruction_t){ "LDA", ZERO_PAGE,   2, 3, op_lda };
    instructions[0xB5] = (instruction_t){ "LDA", ZERO_PAGE_X, 2, 4, op_lda };
    instructions[0xAD] = (instruction_t){ "LDA", ABSOLUTE,    3, 4, op_lda };
    instructions[0xBD] = (instruction_t){ "LDA", ABSOLUTE_X,  3, 4, op_lda };
    instructions[0xB9] = (instruction_t){ "LDA", ABSOLUTE_Y,  3, 4, op_lda };
    instructions[0xA1] = (instruction_t){ "LDA", INDIRECT_X,  2, 6, op_lda };
    instructions[0xB1] = (instruction_t){ "LDA", INDIRECT_Y,  2, 5, op_lda };

    instructions[0xA2] = (instruction_t){ "LDX", IMMEDIATE,   2, 2, op_ldx };
    instructions[0xA6] = (instruction_t){ "LDX", ZERO_PAGE,   2, 3, op_ldx };
    instructions[0xB6] = (instruction_t){ "LDX", ZERO_PAGE_Y, 2, 4, op_ldx };
    instructions[0xAE] = (instruction_t){ "LDX", ABSOLUTE,    3, 4, op_ldx };
    instructions[0xBE] = (instruction_t){ "LDX", ABSOLUTE_Y,  3, 4, op_ldx };

    instructions[0xA0] = (instruction_t){ "LDY", IMMEDIATE,   2, 2, op_ldy };
    instructions[0xA4] = (instruction_t){ "LDY", ZERO_PAGE,   2, 3, op_ldy };
    instructions[0xB4] = (instruction_t){ "LDY", ZERO_PAGE_X, 2, 4, op_ldy };
    instructions[0xAC] = (instruction_t){ "LDY", ABSOLUTE,    3, 4, op_ldy };
    instructions[0xBC] = (instruction_t){ "LDY", ABSOLUTE_X,  3, 4, op_ldy };

    instructions[0x85] = (instruction_t){ "STA", ZERO_PAGE,   2, 3, op_sta };
    instructions[0x95] = (instruction_t){ "STA", ZERO_PAGE_X, 2, 4, op_sta };
    instructions[0x8D] = (instruction_t){ "STA", ABSOLUTE,    3, 4, op_sta };
    instructions[0x9D] = (instruction_t){ "STA", ABSOLUTE_X,  3, 5, op_sta };
    instructions[0x99] = (instruction_t){ "STA", ABSOLUTE_Y,  3, 5, op_sta };
    instructions[0x81] = (instruction_t){ "STA", INDIRECT_X,  2, 6, op_sta };
    instructions[0x91] = (instruction_t){ "STA", INDIRECT_Y,  2, 6, op_sta };

    instructions[0x86] = (instruction_t){ "STX", ZERO_PAGE,   2, 3, op_stx };
    instructions[0x96] = (instruction_t){ "STX", ZERO_PAGE_Y, 2, 4, op_stx };
    instructions[0x8E] = (instruction_t){ "STX", ABSOLUTE,    3, 4, op_stx };

    instructions[0x84] = (instruction_t){ "STY", ZERO_PAGE,   2, 3, op_sty };
    instructions[0x94] = (instruction_t){ "STY", ZERO_PAGE_X, 2, 4, op_sty };
    instructions[0x8C] = (instruction_t){ "STY", ABSOLUTE,    3, 4, op_sty };

    // === TRANSFER ===
    instructions[0xAA] = (instruction_t){ "TAX", IMPLIED, 1, 2, op_tax };
    instructions[0xA8] = (instruction_t){ "TAY", IMPLIED, 1, 2, op_tay };
    instructions[0x8A] = (instruction_t){ "TXA", IMPLIED, 1, 2, op_txa };
    instructions[0x98] = (instruction_t){ "TYA", IMPLIED, 1, 2, op_tya };
    instructions[0xBA] = (instruction_t){ "TSX", IMPLIED, 1, 2, op_tsx };
    instructions[0x9A] = (instruction_t){ "TXS", IMPLIED, 1, 2, op_txs };

    // === JUMPS ===
    instructions[0x4C] = (instruction_t){ "JMP", ABSOLUTE, 3, 3, op_jmp };
    instructions[0x6C] = (instruction_t){ "JMP", INDIRECT, 3, 5, op_jmp };
    instructions[0x20] = (instruction_t){ "JSR", ABSOLUTE, 3, 6, op_jsr };
    instructions[0x60] = (instruction_t){ "RTS", IMPLIED,  1, 6, op_rts };

    // === INTERRUPTS ===
    instructions[0x00] = (instruction_t){ "BRK", IMPLIED, 1, 7, op_brk };
    instructions[0x40] = (instruction_t){ "RTI", IMPLIED, 1, 6, op_rti };

    // === BRANCHES ===
    instructions[0x10] = (instruction_t){ "BPL", RELATIVE, 2, 2, op_bpl };
    instructions[0x30] = (instruction_t){ "BMI", RELATIVE, 2, 2, op_bmi };
    instructions[0x50] = (instruction_t){ "BVC", RELATIVE, 2, 2, op_bvc };
    instructions[0x70] = (instruction_t){ "BVS", RELATIVE, 2, 2, op_bvs };
    instructions[0x90] = (instruction_t){ "BCC", RELATIVE, 2, 2, op_bcc };
    instructions[0xB0] = (instruction_t){ "BCS", RELATIVE, 2, 2, op_bcs };
    instructions[0xD0] = (instruction_t){ "BNE", RELATIVE, 2, 2, op_bne };
    instructions[0xF0] = (instruction_t){ "BEQ", RELATIVE, 2, 2, op_beq };

    // === FLAGS ===
    instructions[0xD8] = (instruction_t){ "CLD", IMPLIED, 1, 2, op_cld };
    instructions[0x58] = (instruction_t){ "CLI", IMPLIED, 1, 2, op_cli };
    instructions[0xB8] = (instruction_t){ "CLV", IMPLIED, 1, 2, op_clv };
    instructions[0x18] = (instruction_t){ "CLC", IMPLIED, 1, 2, op_clc };
    instructions[0xF8] = (instruction_t){ "SED", IMPLIED, 1, 2, op_sed };
    instructions[0x78] = (instruction_t){ "SEI", IMPLIED, 1, 2, op_sei };
    instructions[0x38] = (instruction_t){ "SEC", IMPLIED, 1, 2, op_sec };

    // === MISC ===
    instructions[0xEA] = (instruction_t){ "NOP", IMPLIED, 1, 2, op_nop };

    // === ARITHMETIC ===
    instructions[0x69] = (instruction_t){ "ADC", IMMEDIATE,   2, 2, op_adc };
    instructions[0x65] = (instruction_t){ "ADC", ZERO_PAGE,   2, 3, op_adc };
    instructions[0x75] = (instruction_t){ "ADC", ZERO_PAGE_X, 2, 4, op_adc };
    instructions[0x6D] = (instruction_t){ "ADC", ABSOLUTE,    3, 4, op_adc };
    instructions[0x7D] = (instruction_t){ "ADC", ABSOLUTE_X,  3, 4, op_adc };
    instructions[0x79] = (instruction_t){ "ADC", ABSOLUTE_Y,  3, 4, op_adc };
    instructions[0x61] = (instruction_t){ "ADC", INDIRECT_X,  2, 6, op_adc };
    instructions[0x71] = (instruction_t){ "ADC", INDIRECT_Y,  2, 5, op_adc };

    instructions[0xE9] = (instruction_t){ "SBC", IMMEDIATE,   2, 2, op_sbc };
    instructions[0xE5] = (instruction_t){ "SBC", ZERO_PAGE,   2, 3, op_sbc };
    instructions[0xF5] = (instruction_t){ "SBC", ZERO_PAGE_X, 2, 4, op_sbc };
    instructions[0xED] = (instruction_t){ "SBC", ABSOLUTE,    3, 4, op_sbc };
    instructions[0xFD] = (instruction_t){ "SBC", ABSOLUTE_X,  3, 4, op_sbc };
    instructions[0xF9] = (instruction_t){ "SBC", ABSOLUTE_Y,  3, 4, op_sbc };
    instructions[0xE1] = (instruction_t){ "SBC", INDIRECT_X,  2, 6, op_sbc };
    instructions[0xF1] = (instruction_t){ "SBC", INDIRECT_Y,  2, 5, op_sbc };

    // === LOGICAL ===
    instructions[0x29] = (instruction_t){ "AND", IMMEDIATE,   2, 2, op_and };
    instructions[0x25] = (instruction_t){ "AND", ZERO_PAGE,   2, 3, op_and };
    instructions[0x35] = (instruction_t){ "AND", ZERO_PAGE_X, 2, 4, op_and };
    instructions[0x2D] = (instruction_t){ "AND", ABSOLUTE,    3, 4, op_and };
    instructions[0x3D] = (instruction_t){ "AND", ABSOLUTE_X,  3, 4, op_and };
    instructions[0x39] = (instruction_t){ "AND", ABSOLUTE_Y,  3, 4, op_and };
    instructions[0x21] = (instruction_t){ "AND", INDIRECT_X,  2, 6, op_and };
    instructions[0x31] = (instruction_t){ "AND", INDIRECT_Y,  2, 5, op_and };

    instructions[0x09] = (instruction_t){ "ORA", IMMEDIATE,   2, 2, op_ora };
    instructions[0x05] = (instruction_t){ "ORA", ZERO_PAGE,   2, 3, op_ora };
    instructions[0x15] = (instruction_t){ "ORA", ZERO_PAGE_X, 2, 4, op_ora };
    instructions[0x0D] = (instruction_t){ "ORA", ABSOLUTE,    3, 4, op_ora };
    instructions[0x1D] = (instruction_t){ "ORA", ABSOLUTE_X,  3, 4, op_ora };
    instructions[0x19] = (instruction_t){ "ORA", ABSOLUTE_Y,  3, 4, op_ora };
    instructions[0x01] = (instruction_t){ "ORA", INDIRECT_X,  2, 6, op_ora };
    instructions[0x11] = (instruction_t){ "ORA", INDIRECT_Y,  2, 5, op_ora };

    instructions[0x49] = (instruction_t){ "EOR", IMMEDIATE,   2, 2, op_eor };
    instructions[0x45] = (instruction_t){ "EOR", ZERO_PAGE,   2, 3, op_eor };
    instructions[0x55] = (instruction_t){ "EOR", ZERO_PAGE_X, 2, 4, op_eor };
    instructions[0x4D] = (instruction_t){ "EOR", ABSOLUTE,    3, 4, op_eor };
    instructions[0x5D] = (instruction_t){ "EOR", ABSOLUTE_X,  3, 4, op_eor };
    instructions[0x59] = (instruction_t){ "EOR", ABSOLUTE_Y,  3, 4, op_eor };
    instructions[0x41] = (instruction_t){ "EOR", INDIRECT_X,  2, 6, op_eor };
    instructions[0x51] = (instruction_t){ "EOR", INDIRECT_Y,  2, 5, op_eor };

    // === COMPARE ===
    instructions[0xC9] = (instruction_t){ "CMP", IMMEDIATE,   2, 2, op_cmp };
    instructions[0xC5] = (instruction_t){ "CMP", ZERO_PAGE,   2, 3, op_cmp };
    instructions[0xD5] = (instruction_t){ "CMP", ZERO_PAGE_X, 2, 4, op_cmp };
    instructions[0xCD] = (instruction_t){ "CMP", ABSOLUTE,    3, 4, op_cmp };
    instructions[0xDD] = (instruction_t){ "CMP", ABSOLUTE_X,  3, 4, op_cmp };
    instructions[0xD9] = (instruction_t){ "CMP", ABSOLUTE_Y,  3, 4, op_cmp };
    instructions[0xC1] = (instruction_t){ "CMP", INDIRECT_X,  2, 6, op_cmp };
    instructions[0xD1] = (instruction_t){ "CMP", INDIRECT_Y,  2, 5, op_cmp };

    instructions[0xE0] = (instruction_t){ "CPX", IMMEDIATE, 2, 2, op_cpx };
    instructions[0xE4] = (instruction_t){ "CPX", ZERO_PAGE, 2, 3, op_cpx };
    instructions[0xEC] = (instruction_t){ "CPX", ABSOLUTE,  3, 4, op_cpx };

    instructions[0xC0] = (instruction_t){ "CPY", IMMEDIATE, 2, 2, op_cpy };
    instructions[0xC4] = (instruction_t){ "CPY", ZERO_PAGE, 2, 3, op_cpy };
    instructions[0xCC] = (instruction_t){ "CPY", ABSOLUTE,  3, 4, op_cpy };

    // === INCREMENT/DECREMENT ===
    instructions[0xE6] = (instruction_t){ "INC", ZERO_PAGE,   2, 5, op_inc };
    instructions[0xF6] = (instruction_t){ "INC", ZERO_PAGE_X, 2, 6, op_inc };
    instructions[0xEE] = (instruction_t){ "INC", ABSOLUTE,    3, 6, op_inc };
    instructions[0xFE] = (instruction_t){ "INC", ABSOLUTE_X,  3, 7, op_inc };

    instructions[0xE8] = (instruction_t){ "INX", IMPLIED, 1, 2, op_inx };
    instructions[0xC8] = (instruction_t){ "INY", IMPLIED, 1, 2, op_iny };

    instructions[0xC6] = (instruction_t){ "DEC", ZERO_PAGE,   2, 5, op_dec };
    instructions[0xD6] = (instruction_t){ "DEC", ZERO_PAGE_X, 2, 6, op_dec };
    instructions[0xCE] = (instruction_t){ "DEC", ABSOLUTE,    3, 6, op_dec };
    instructions[0xDE] = (instruction_t){ "DEC", ABSOLUTE_X,  3, 7, op_dec };

    instructions[0xCA] = (instruction_t){ "DEX", IMPLIED, 1, 2, op_dex };
    instructions[0x88] = (instruction_t){ "DEY", IMPLIED, 1, 2, op_dey };

    // === SHIFTS ===
    instructions[0x0A] = (instruction_t){ "ASL", ACCUMULATOR, 1, 2, op_asl };
    instructions[0x06] = (instruction_t){ "ASL", ZERO_PAGE,   2, 5, op_asl };
    instructions[0x16] = (instruction_t){ "ASL", ZERO_PAGE_X, 2, 6, op_asl };
    instructions[0x0E] = (instruction_t){ "ASL", ABSOLUTE,    3, 6, op_asl };
    instructions[0x1E] = (instruction_t){ "ASL", ABSOLUTE_X,  3, 7, op_asl };

    instructions[0x4A] = (instruction_t){ "LSR", ACCUMULATOR, 1, 2, op_lsr };
    instructions[0x46] = (instruction_t){ "LSR", ZERO_PAGE,   2, 5, op_lsr };
    instructions[0x56] = (instruction_t){ "LSR", ZERO_PAGE_X, 2, 6, op_lsr };
    instructions[0x4E] = (instruction_t){ "LSR", ABSOLUTE,    3, 6, op_lsr };
    instructions[0x5E] = (instruction_t){ "LSR", ABSOLUTE_X,  3, 7, op_lsr };

    instructions[0x2A] = (instruction_t){ "ROL", ACCUMULATOR, 1, 2, op_rol };
    instructions[0x26] = (instruction_t){ "ROL", ZERO_PAGE,   2, 5, op_rol };
    instructions[0x36] = (instruction_t){ "ROL", ZERO_PAGE_X, 2, 6, op_rol };
    instructions[0x2E] = (instruction_t){ "ROL", ABSOLUTE,    3, 6, op_rol };
    instructions[0x3E] = (instruction_t){ "ROL", ABSOLUTE_X,  3, 7, op_rol };

    instructions[0x6A] = (instruction_t){ "ROR", ACCUMULATOR, 1, 2, op_ror };
    instructions[0x66] = (instruction_t){ "ROR", ZERO_PAGE,   2, 5, op_ror };
    instructions[0x76] = (instruction_t){ "ROR", ZERO_PAGE_X, 2, 6, op_ror };
    instructions[0x6E] = (instruction_t){ "ROR", ABSOLUTE,    3, 6, op_ror };
    instructions[0x7E] = (instruction_t){ "ROR", ABSOLUTE_X,  3, 7, op_ror };

    // === STACK ===
    instructions[0x48] = (instruction_t){ "PHA", IMPLIED, 1, 3, op_pha };
    instructions[0x68] = (instruction_t){ "PLA", IMPLIED, 1, 4, op_pla };
    instructions[0x08] = (instruction_t){ "PHP", IMPLIED, 1, 3, op_php };
    instructions[0x28] = (instruction_t){ "PLP", IMPLIED, 1, 4, op_plp };

    // === BIT TEST ===
    instructions[0x24] = (instruction_t){ "BIT", ZERO_PAGE, 2, 3, op_bit };
    instructions[0x2C] = (instruction_t){ "BIT", ABSOLUTE,  3, 4, op_bit };

    printf("[CPU] Tabela de instruções inicializada com sucesso!\n");
}
