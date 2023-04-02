#include "opcodes.h"

#include <stddef.h>
#include <stdbool.h>

#include "regs.h"

/*
 * Reference: https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf
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
        \
        mod = (READ_MOD_REG_RM_cmd & 0xC0) >> 6;\
        reg = (READ_MOD_REG_RM_cmd & 0x38) >> 3;\
        r_m = READ_MOD_REG_RM_cmd & 0x07;\
    } while(0)

inline Opcode_Err read_error_code(BinFileReader *reader) {
    if(BinFileReader_status(reader) == BinFileReader_Status_ERROR) {
        return Opcode_Err_FILE_ERR;
    }
    return Opcode_Err_EOF;
}

inline Opcode_Err read_int8(char **dst, BinFileReader *reader) {
    int8_t addr;
    READ_BYTE(addr, reader);

    *dst += sprintf(*dst, "%d", addr);
    return Opcode_Err_OK;
}

inline Opcode_Err read_int16(char **dst, BinFileReader *reader) {
    int16_t addr;
    READ_WORD(addr, reader);

    *dst += sprintf(*dst, "%d", addr);
    return Opcode_Err_OK;
}

inline bool isMemoryMode(uint8_t mod) {
    return mod == 0x00 || mod == 0x01 || mod == 0x02;
}

inline bool isRegisterMode(uint8_t mod) {
    return mod == 0x03;
}

int effective_addr_base(char *dst, uint8_t r_m) {
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

Opcode_Err resolve_mem_addr(char **dst, BinFileReader *reader, uint8_t mod, uint8_t r_m) {
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

inline Opcode_Err resolve_reg(char **dst, uint8_t  reg, bool w) {
    *dst += sprintf(*dst, "%s", w ? wordRegs[reg] : byteRegs[reg]);
    return Opcode_Err_OK;
}

inline Opcode_Err resolve_rm_addr(char **dst, BinFileReader *reader, uint8_t mod, uint8_t r_m, bool w) {
    return isMemoryMode(mod)
        ? resolve_mem_addr(dst, reader, mod, r_m)
        : resolve_reg(dst, r_m, w)
        ;
}

inline Opcode_Err resolve_immediate_val(char **dst, BinFileReader *reader, bool w) {
    return w
        ? read_int16(dst, reader)
        : read_int8(dst, reader)
        ;
}

/* -------------------- OPCODES ---------------------- */

// MOV register/memory to/from register
Opcode_Err mov_rm_tf_reg(FILE *out, BinFileReader *reader, uint8_t cmd) {
    char srcBuf[MAX_ARG_LEN], dstBuf[MAX_ARG_LEN];

    bool d = (cmd & 0x02) >> 1; // 0b00000010
    bool w = cmd & 0x01;        // 0b00000001

    char *regArg = d ? dstBuf : srcBuf;
    char *r_mArg = !d ? dstBuf : srcBuf;

    uint8_t mod, reg, r_m;
    READ_MOD_REG_RM(reader, mod, reg, r_m);

    CHECK_ERR(resolve_reg(&regArg, reg, w));
    CHECK_ERR(resolve_rm_addr(&r_mArg, reader, mod, r_m, w));

    *regArg = 0;
    *r_mArg = 0;

    fprintf(out, "mov %s, %s\n", dstBuf, srcBuf);
    return Opcode_Err_OK;
}

// MOV immediate to register/memory
Opcode_Err mov_imm_to_rm(FILE *out, BinFileReader *reader, uint8_t cmd) {
    char srcBuf[MAX_ARG_LEN], dstBuf[MAX_ARG_LEN];
    char *immArg = srcBuf, *r_mArg = dstBuf;

    bool w = cmd & 0x01; // 0b00000001

    uint8_t mod, reg, r_m;
    READ_MOD_REG_RM(reader, mod, reg, r_m);

    if(reg != 0x00) {
        return Opcode_Err_INVALID_ARG;
    }

    CHECK_ERR(resolve_rm_addr(&r_mArg, reader, mod, r_m, w));

    if(isMemoryMode(mod)) {
        // Need explicit size on immediate value
        immArg += sprintf(immArg, w ? "word " : "byte ");
    }
    CHECK_ERR(resolve_immediate_val(&immArg, reader, w));

    *immArg = 0;
    *r_mArg = 0;

    fprintf(out, "mov %s, %s\n", dstBuf, srcBuf);
    return Opcode_Err_OK;
}

// MOV immediate to register
Opcode_Err mov_imm_to_reg(FILE *out, BinFileReader *reader, uint8_t cmd) {
    char srcBuf[MAX_ARG_LEN], dstBuf[MAX_ARG_LEN];
    char *immArg = srcBuf, *regArg = dstBuf;

    bool w = cmd & 0x08;        // 0b00001000
    uint8_t reg = cmd & 0x07;   // 0b00000111

    CHECK_ERR(resolve_reg(&regArg, reg, w));
    CHECK_ERR(resolve_immediate_val(&immArg, reader, w));

    *immArg = 0;
    *regArg = 0;

    fprintf(out, "mov %s, %s\n", dstBuf, srcBuf);
    return Opcode_Err_OK;
}

// MOV memory to/from accumulator
Opcode_Err mov_mem_tf_acc(FILE *out, BinFileReader *reader, uint8_t cmd) {
    char addrBuf[MAX_ARG_LEN];
    char *addr = addrBuf;

    bool d = (cmd & 0x02) >> 1; // 0b00000010
    bool w = cmd & 0x01;        // 0b00000001

    const char *acc = w ? wordRegs[REG_AX] : byteRegs[REG_AL];
    CHECK_ERR(read_int16(&addr, reader));

    *addr = 0;

    if(d) {
        fprintf(out, "mov [%s], %s\n", addrBuf, acc);
    } else {
        fprintf(out, "mov %s, [%s]\n", acc, addrBuf);
    }

    return Opcode_Err_OK;
}

Opcode opcodes[OPCODE_COUNT] = {
        // 0b100010|d|w: MOV register/memory to/from register
        [0x88] = mov_rm_tf_reg,
        [0x89] = mov_rm_tf_reg,
        [0x8A] = mov_rm_tf_reg,
        [0x8B] = mov_rm_tf_reg,

        // 0b1100011|w: MOV immediate to register/memory
        [0xC6] = mov_imm_to_rm,
        [0xC7] = mov_imm_to_rm,

        // 0b1011|w|reg: MOV immediate to register
        [0xB0] = mov_imm_to_reg,
        [0xB1] = mov_imm_to_reg,
        [0xB2] = mov_imm_to_reg,
        [0xB3] = mov_imm_to_reg,
        [0xB4] = mov_imm_to_reg,
        [0xB5] = mov_imm_to_reg,
        [0xB6] = mov_imm_to_reg,
        [0xB7] = mov_imm_to_reg,
        [0xB8] = mov_imm_to_reg,
        [0xB9] = mov_imm_to_reg,
        [0xBA] = mov_imm_to_reg,
        [0xBB] = mov_imm_to_reg,
        [0xBC] = mov_imm_to_reg,
        [0xBD] = mov_imm_to_reg,
        [0xBE] = mov_imm_to_reg,
        [0xBF] = mov_imm_to_reg,

        // 0b101000|d|w: MOV memory to/from accumulator
        [0xA0] = mov_mem_tf_acc,
        [0xA1] = mov_mem_tf_acc,
        [0xA2] = mov_mem_tf_acc,
        [0xA3] = mov_mem_tf_acc,
};

#undef MAX_ARG_LEN
#undef CHECK_ERR
#undef READ_BYTE
#undef READ_WORD
#undef READ_MOD_REG_RM
