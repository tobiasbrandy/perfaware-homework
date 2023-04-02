#include "opcodes.h"

#include <stddef.h>
#include <stdbool.h>

#include "regs.h"

/*
 * Reference: https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf
 *
 * s = 0: No sign extension.
 * s = 1: Sign extend 8-bit immediate data to 16 bits if w = 1.
 *
 * w = 0: Instruction operates in byte data.
 * w = 1: Instruction operates in word data.
 *
 * d = 0: Instruction source is specified in REG field.
 * d = 1: Instruction destination is specified in REG field.
 *
 * mod = 00: Memory Mode, no displacement follows (expect R/M = 110, then 16 bit displacement follows).
 * mod = 01: Memory Mode, 8-bit displacement follows.
 * mod = 10: Memory Mode, 16-bit displacement follows.
 * mod = 11: Register Mode (no displacement).
 */

#define MAX_ARG_LEN 23 // Example: `word [bp + di - 10044]\0`

#define CHECK_ERR(exp) do { \
    Opcode_Err CHECK_ERR_err; \
    if((CHECK_ERR_err = exp)) { \
        return CHECK_ERR_err; \
    } \
    } while(0)

#define PARSE_DS_W(cmd, d_s, w) do { \
    d_s = (cmd & 0x02) >> 1; \
    w = cmd & 0x01; \
    } while(0)

#define PARSE_MOD_REG_RM(cmd, mod, reg, r_m) do { \
    mod = (READ_MOD_REG_RM_cmd & 0xC0) >> 6; \
    reg = (READ_MOD_REG_RM_cmd & 0x38) >> 3; \
    r_m = READ_MOD_REG_RM_cmd & 0x07; \
    } while(0)

#define READ_BYTE(out, reader) do { \
    uint8_t *READ_BYTE_tmp; \
    if((READ_BYTE_tmp = BinFileReader_read_bytes(reader, 1)) == NULL) { \
        return read_error_code(reader); \
    } \
    out = *READ_BYTE_tmp; \
    } while(0)

#define READ_WORD(out, reader) do { \
    uint8_t *READ_WORD_tmp; \
    if((READ_WORD_tmp = BinFileReader_read_bytes(reader, 2)) == NULL) { \
        return read_error_code(reader); \
    } \
    out = (READ_WORD_tmp[1] << 8) + READ_WORD_tmp[0];\
    } while(0)

#define READ_MOD_REG_RM(reader, mod, reg, r_m) do {\
        uint8_t READ_MOD_REG_RM_cmd;\
        READ_BYTE(READ_MOD_REG_RM_cmd, reader);\
        PARSE_MOD_REG_RM(READ_MOD_REG_RM_cmd, mod, reg, r_m); \
    } while(0)

static inline Opcode_Err read_error_code(BinFileReader *reader) {
    if(BinFileReader_status(reader) == BinFileReader_Status_ERROR) {
        return Opcode_Err_FILE_ERR;
    }
    return Opcode_Err_EOF;
}

static inline Opcode_Err read_int8(char **dst, BinFileReader *reader) {
    int8_t addr;
    READ_BYTE(addr, reader);

    *dst += sprintf(*dst, "%d", addr);
    return Opcode_Err_OK;
}

static inline Opcode_Err read_int16(char **dst, BinFileReader *reader) {
    int16_t addr;
    READ_WORD(addr, reader);

    *dst += sprintf(*dst, "%d", addr);
    return Opcode_Err_OK;
}

static inline bool isMemoryMode(uint8_t mod) {
    return mod == 0x00 || mod == 0x01 || mod == 0x02;
}

static int effective_addr_base(char *dst, uint8_t r_m) {
    switch(r_m) {
        case 0x00: return sprintf(dst, "%s + %s", wordRegs[REG_BX], wordRegs[REG_SI]);
        case 0x01: return sprintf(dst, "%s + %s", wordRegs[REG_BX], wordRegs[REG_DI]);
        case 0x02: return sprintf(dst, "%s + %s", wordRegs[REG_BP], wordRegs[REG_SI]);
        case 0x03: return sprintf(dst, "%s + %s", wordRegs[REG_BP], wordRegs[REG_DI]);
        case 0x04: return sprintf(dst, "%s", wordRegs[REG_SI]);
        case 0x05: return sprintf(dst, "%s", wordRegs[REG_DI]);
        case 0x06: return sprintf(dst, "%s", wordRegs[REG_BP]);
        case 0x07: return sprintf(dst, "%s", wordRegs[REG_BX]);
        default: return 0;
    }
}

static Opcode_Err resolve_mem_addr(char **dst, BinFileReader *reader, uint8_t mod, uint8_t r_m) {
    *(*dst)++ = '[';

    if(mod == 0x00 && r_m == 0x06) {
        // Direct Address
        CHECK_ERR(read_int16(dst, reader));
        goto end;
    }

    *dst += effective_addr_base(*dst, r_m);

    if(mod == 0x00) {
        // No displacement
        goto end;
    }

    int displacement;
    if(mod == 0x01) {
        // 8-bit displacement
        int8_t tmp;
        READ_BYTE(tmp, reader);
        displacement = (int16_t) tmp;
    } else {
        // 16-bit displacement
        int16_t tmp;
        READ_WORD(tmp, reader);
        displacement = tmp;
    }

    char sign;
    if(displacement >= 0) {
        sign = '+';
    } else {
        sign = '-';
        displacement = -displacement;
    }

    *dst += sprintf(*dst, " %c %d", sign, displacement);

    end:
    *(*dst)++ = ']';
    return Opcode_Err_OK;
}

static inline Opcode_Err resolve_reg(char **dst, uint8_t reg, bool w) {
    *dst += sprintf(*dst, "%s", w ? wordRegs[reg] : byteRegs[reg]);
    return Opcode_Err_OK;
}

static inline Opcode_Err resolve_rm_addr(char **dst, BinFileReader *reader, uint8_t mod, uint8_t r_m, bool w) {
    return isMemoryMode(mod) ? resolve_mem_addr(dst, reader, mod, r_m) : resolve_reg(dst, r_m, w);
}

static inline Opcode_Err resolve_immediate_val(char **dst, BinFileReader *reader, bool s, bool w) {
    if(s && w) {
        // Sign extend 8-bit immediate data to 16 bits
        int8_t imm;
        READ_BYTE(imm, reader);

        *dst += sprintf(*dst, "%d", (int16_t) imm);
        return Opcode_Err_OK;
    }

    return w ? read_int16(dst, reader) : read_int8(dst, reader);
}

/* -------------------- GENERALIZED OPCODES ---------------------- */

// OP register/memory to/from register
static Opcode_Err op_rm_tf_reg(const char *op, FILE *out, BinFileReader *reader, uint8_t cmd) {
    char srcBuf[MAX_ARG_LEN], dstBuf[MAX_ARG_LEN];

    bool d, w;
    PARSE_DS_W(cmd, d, w);

    char *regArg = d ? dstBuf : srcBuf;
    char *r_mArg = !d ? dstBuf : srcBuf;

    uint8_t mod, reg, r_m;
    READ_MOD_REG_RM(reader, mod, reg, r_m);

    CHECK_ERR(resolve_reg(&regArg, reg, w));
    *regArg = 0;

    CHECK_ERR(resolve_rm_addr(&r_mArg, reader, mod, r_m, w));
    *r_mArg = 0;

    fprintf(out, "%s %s, %s\n", op, dstBuf, srcBuf);
    return Opcode_Err_OK;
}

// OP immediate to register/memory
static Opcode_Err op_imm_to_rm(const char *op, FILE *out, BinFileReader *reader, uint8_t mod, uint8_t r_m, bool s, bool w) {
    char srcBuf[MAX_ARG_LEN], dstBuf[MAX_ARG_LEN];
    char *immArg = srcBuf, *r_mArg = dstBuf;

    CHECK_ERR(resolve_rm_addr(&r_mArg, reader, mod, r_m, w));
    *r_mArg = 0;

    if(isMemoryMode(mod)) {
        // Need explicit size on immediate value
        immArg += sprintf(immArg, w ? "word " : "byte ");
    }
    CHECK_ERR(resolve_immediate_val(&immArg, reader, s, w));
    *immArg = 0;

    fprintf(out, "%s %s, %s\n", op, dstBuf, srcBuf);
    return Opcode_Err_OK;
}

static Opcode_Err op_imm_to_acc(const char *op, FILE *out, BinFileReader *reader, uint8_t cmd) {
    char immBuf[MAX_ARG_LEN];
    char *immArg = immBuf;

    bool w = cmd & 1;

    const char *acc = w ? wordRegs[REG_AX] : byteRegs[REG_AL];

    CHECK_ERR(resolve_immediate_val(&immArg, reader, 0, w));
    *immArg = 0;

    fprintf(out, "%s %s, %s\n", op, acc, immBuf);
    return Opcode_Err_OK;
}

/* -------------------- OPCODES ---------------------- */

/* ------- ARITH (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP) ------- */
static const char *arithCodeToOp[8] = {
        [0] = "add", [1] = "or", [2] = "adc",
        [3] = "sbb", [4] = "and", [5] = "sub",
        [6] = "xor", [7] = "cmp",
};

static inline const char *arith_op_name(uint8_t cmd) {
    return arithCodeToOp[(cmd & 0x38) >> 3]; // 0b00111000
}

// ARITH register/memory with register to either
static Opcode_Err arith_rm_tf_reg(FILE *out, BinFileReader *reader, uint8_t cmd) {
    return op_rm_tf_reg(arith_op_name(cmd), out, reader, cmd);
}

// ARITH immediate to register/memory
static Opcode_Err arith_imm_to_rm(FILE *out, BinFileReader *reader, uint8_t cmd) {
    bool s, w;
    PARSE_DS_W(cmd, s, w);

    uint8_t mod, code, r_m;
    READ_MOD_REG_RM(reader, mod, code, r_m);

    return op_imm_to_rm(arithCodeToOp[code], out, reader, mod, r_m, s, w);
}

// ARITH immediate to accumulator
static Opcode_Err arith_imm_to_acc(FILE *out, BinFileReader *reader, uint8_t cmd) {
    return op_imm_to_acc(arith_op_name(cmd), out, reader, cmd);
}

/* ------- MOV ------- */

// MOV register/memory to/from register
static Opcode_Err mov_rm_tf_reg(FILE *out, BinFileReader *reader, uint8_t cmd) {
    return op_rm_tf_reg("mov", out, reader, cmd);
}

// MOV immediate to register/memory
static Opcode_Err mov_imm_to_rm(FILE *out, BinFileReader *reader, uint8_t cmd) {
    bool w = cmd & 1;

    uint8_t mod, reg, r_m;
    READ_MOD_REG_RM(reader, mod, reg, r_m);

    if(reg != 0) {
        return Opcode_Err_INVALID_ARG;
    }

    return op_imm_to_rm("mov", out, reader, mod, r_m, 0, w);
}

// MOV immediate to register
static Opcode_Err mov_imm_to_reg(FILE *out, BinFileReader *reader, uint8_t cmd) {
    char srcBuf[MAX_ARG_LEN], dstBuf[MAX_ARG_LEN];
    char *immArg = srcBuf, *regArg = dstBuf;

    bool w = cmd & 0x08;        // 0b00001000
    uint8_t reg = cmd & 0x07;   // 0b00000111

    CHECK_ERR(resolve_reg(&regArg, reg, w));
    *regArg = 0;

    CHECK_ERR(resolve_immediate_val(&immArg, reader, 0, w));
    *immArg = 0;

    fprintf(out, "mov %s, %s\n", dstBuf, srcBuf);
    return Opcode_Err_OK;
}

// MOV memory to/from accumulator
static Opcode_Err mov_mem_tf_acc(FILE *out, BinFileReader *reader, uint8_t cmd) {
    char addrBuf[MAX_ARG_LEN];
    char *addrArg = addrBuf;

    bool d, w;
    PARSE_DS_W(cmd, d, w);

    const char *acc = w ? wordRegs[REG_AX] : byteRegs[REG_AL];

    CHECK_ERR(read_int16(&addrArg, reader));
    *addrArg = 0;

    if(d) {
        fprintf(out, "mov [%s], %s\n", addrBuf, acc);
    } else {
        fprintf(out, "mov %s, [%s]\n", acc, addrBuf);
    }

    return Opcode_Err_OK;
}

/* ------- JMP ------- */
static Opcode_Err op_jmp_8bit_inc(const char *op, FILE *out, BinFileReader *reader) {
    // inc = increment before jmp instruction
    int8_t inc;
    READ_BYTE(inc, reader);

    // realInc = increment after jmp instruction
    int realInc = inc + 2;

    if(realInc >= 0) {
        fprintf(out, "%s $+%d\n", op, realInc);
    } else {
        fprintf(out, "%s $%d\n", op, realInc);
    }

    return Opcode_Err_OK;
}

static Opcode_Err je(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("je", out, reader); }
static Opcode_Err jl(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jl", out, reader); }
static Opcode_Err jle(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jle", out, reader); }
static Opcode_Err jb(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jb", out, reader); }
static Opcode_Err jbe(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jbe", out, reader); }
static Opcode_Err jp(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jp", out, reader); }
static Opcode_Err jo(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jo", out, reader); }
static Opcode_Err js(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("js", out, reader); }
static Opcode_Err jne(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jne", out, reader); }
static Opcode_Err jnl(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jnl", out, reader); }
static Opcode_Err jnle(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jnle", out, reader); }
static Opcode_Err jnb(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jnb", out, reader); }
static Opcode_Err jnbe(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jnbe", out, reader); }
static Opcode_Err jnp(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jnp", out, reader); }
static Opcode_Err jno(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jno", out, reader); }
static Opcode_Err jns(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jns", out, reader); }
static Opcode_Err loop(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("loop", out, reader); }
static Opcode_Err loope(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("loope", out, reader); }
static Opcode_Err loopne(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("loopne", out, reader); }
static Opcode_Err jcxz(FILE *out, BinFileReader *reader, uint8_t _) { return op_jmp_8bit_inc("jcxz", out, reader); }

Opcode opcodes[OPCODE_COUNT] = {
        /* ------- ARITH (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP) ------- */
        // 0b00|opc|0|d|w: ARITH register/memory with register to either
        [0x00] = arith_rm_tf_reg, [0x01] = arith_rm_tf_reg, [0x02] = arith_rm_tf_reg, [0x03] = arith_rm_tf_reg, // ADD
        [0x08] = arith_rm_tf_reg, [0x09] = arith_rm_tf_reg, [0x0A] = arith_rm_tf_reg, [0x0B] = arith_rm_tf_reg, // OR
        [0x10] = arith_rm_tf_reg, [0x11] = arith_rm_tf_reg, [0x12] = arith_rm_tf_reg, [0x13] = arith_rm_tf_reg, // ADC
        [0x18] = arith_rm_tf_reg, [0x19] = arith_rm_tf_reg, [0x1A] = arith_rm_tf_reg, [0x1B] = arith_rm_tf_reg, // SBB
        [0x20] = arith_rm_tf_reg, [0x21] = arith_rm_tf_reg, [0x22] = arith_rm_tf_reg, [0x23] = arith_rm_tf_reg, // AND
        [0x28] = arith_rm_tf_reg, [0x29] = arith_rm_tf_reg, [0x2A] = arith_rm_tf_reg, [0x2B] = arith_rm_tf_reg, // SUB
        [0x30] = arith_rm_tf_reg, [0x31] = arith_rm_tf_reg, [0x32] = arith_rm_tf_reg, [0x33] = arith_rm_tf_reg, // XOR
        [0x38] = arith_rm_tf_reg, [0x39] = arith_rm_tf_reg, [0x3A] = arith_rm_tf_reg, [0x3B] = arith_rm_tf_reg, // CMP

        // 0b100000|s|w: ARITH immediate to register/memory
        [0x80] = arith_imm_to_rm, [0x81] = arith_imm_to_rm, [0x82] = arith_imm_to_rm, [0x83] = arith_imm_to_rm,

        // 0b00|opc|10|w: ARITH immediate to accumulator
        [0x04] = arith_imm_to_acc, [0x05] = arith_imm_to_acc, // ADD
        [0x0C] = arith_imm_to_acc, [0x0D] = arith_imm_to_acc, // OR
        [0x14] = arith_imm_to_acc, [0x15] = arith_imm_to_acc, // ADC
        [0x1C] = arith_imm_to_acc, [0x1D] = arith_imm_to_acc, // SBB
        [0x24] = arith_imm_to_acc, [0x25] = arith_imm_to_acc, // AND
        [0x2C] = arith_imm_to_acc, [0x2D] = arith_imm_to_acc, // SUB
        [0x34] = arith_imm_to_acc, [0x35] = arith_imm_to_acc, // XOR
        [0x3C] = arith_imm_to_acc, [0x3D] = arith_imm_to_acc, // CMP

        /* ------- MOV ------- */
        // 0b100010|d|w: MOV register/memory to/from register
        [0x88] = mov_rm_tf_reg, [0x89] = mov_rm_tf_reg, [0x8A] = mov_rm_tf_reg, [0x8B] = mov_rm_tf_reg,

        // 0b1100011|w: MOV immediate to register/memory
        [0xC6] = mov_imm_to_rm, [0xC7] = mov_imm_to_rm,

        // 0b1011|w|reg: MOV immediate to register
        [0xB0] = mov_imm_to_reg, [0xB1] = mov_imm_to_reg, [0xB2] = mov_imm_to_reg, [0xB3] = mov_imm_to_reg,
        [0xB4] = mov_imm_to_reg, [0xB5] = mov_imm_to_reg, [0xB6] = mov_imm_to_reg, [0xB7] = mov_imm_to_reg,
        [0xB8] = mov_imm_to_reg, [0xB9] = mov_imm_to_reg, [0xBA] = mov_imm_to_reg, [0xBB] = mov_imm_to_reg,
        [0xBC] = mov_imm_to_reg, [0xBD] = mov_imm_to_reg, [0xBE] = mov_imm_to_reg, [0xBF] = mov_imm_to_reg,

        // 0b101000|d|w: MOV memory to/from accumulator
        [0xA0] = mov_mem_tf_acc, [0xA1] = mov_mem_tf_acc, [0xA2] = mov_mem_tf_acc, [0xA3] = mov_mem_tf_acc,

        /* ------- JMP ------- */
        [0x74] = je,    [0x7C] = jl,    [0x7E] = jle,   [0x72] = jb,    [0x76] = jbe,   [0x7A] = jp,    [0x70] = jo,
        [0x78] = js,    [0x75] = jne,   [0x7D] = jnl,   [0x7F] = jnle,  [0x73] = jnb,   [0x77] = jnbe,  [0x7B] = jnp,
        [0x71] = jno,   [0x79] = jns,   [0xE2] = loop,  [0xE1] = loope, [0xE0] = loopne,[0xE3] = jcxz,
};

#undef MAX_ARG_LEN
#undef CHECK_ERR
#undef PARSE_DS_W
#undef PARSE_MOD_REG_RM
#undef READ_BYTE
#undef READ_WORD
#undef READ_MOD_REG_RM
