#ifndef PERFAWARE_REGS_H
#define PERFAWARE_REGS_H

#define REG_COUNT 8

// Word registers
#define REG_AX 0x0 // 0b000
#define REG_CX 0x1 // 0b001
#define REG_DX 0x2 // 0b010
#define REG_BX 0x3 // 0b011
#define REG_SP 0x4 // 0b100
#define REG_BP 0x5 // 0b101
#define REG_SI 0x6 // 0b110
#define REG_DI 0x7 // 0b111

// Byte registers
#define REG_AL 0x0 // 0b000
#define REG_CL 0x1 // 0b001
#define REG_DL 0x2 // 0b010
#define REG_BL 0x3 // 0b011
#define REG_AH 0x4 // 0b100
#define REG_CH 0x5 // 0b101
#define REG_DH 0x6 // 0b110
#define REG_BH 0x7 // 0b111

const char *wordRegs[REG_COUNT] = {
        [REG_AX] = "ax",
        [REG_CX] = "cx",
        [REG_DX] = "dx",
        [REG_BX] = "bx",
        [REG_SP] = "sp",
        [REG_BP] = "bp",
        [REG_SI] = "si",
        [REG_DI] = "di",
};

const char *byteRegs[REG_COUNT] = {
        [REG_AL] = "al",
        [REG_CL] = "cl",
        [REG_DL] = "dl",
        [REG_BL] = "bl",
        [REG_AH] = "ah",
        [REG_CH] = "ch",
        [REG_DH] = "dh",
        [REG_BH] = "bh",
};

#endif //PERFAWARE_REGS_H
