#include "opcodes.h"

#include <stddef.h>
#include <stdbool.h>

#include "regs.h"

/*
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

#define READ_BYTE(reader, out) do { \
    uint8_t *MACRO_tmp;\
    if((MACRO_tmp = BinFileReader_read_bytes(reader, 1)) == NULL) {\
        return read_error_code(reader);\
    }\
    out = *MACRO_tmp;\
    } while(0)

#define READ_WORD(reader, out) do { \
    uint8_t *MACRO_tmp;\
    if((MACRO_tmp = BinFileReader_read_bytes(reader, 2)) == NULL) {\
        return read_error_code(reader);\
    }\
    out = (MACRO_tmp[1] << 8) + MACRO_tmp[0];\
    } while(0)

#define READ_MOD_REG_RM(reader, mod, reg, r_m) do { \
        uint8_t MACRO_cmd;\
        READ_BYTE(reader, MACRO_cmd);\
        \
        mod = (MACRO_cmd & 0xC0) >> 6;\
        reg = (MACRO_cmd & 0x38) >> 3;\
        r_m = MACRO_cmd & 0x07;\
    } while(0)

Opcode_Err read_error_code(BinFileReader *reader) {
    if(BinFileReader_status(reader) == BinFileReader_Status_ERROR) {
        return Opcode_Err_FILE_ERR;
    }
    return Opcode_Err_EOF;
}

int read_8_bit_val(BinFileReader *reader, char *dst) {
    uint8_t addr;
    READ_BYTE(reader, addr);

    return sprintf(dst, "%d", addr);
}

int read_16_bit_val(BinFileReader *reader, char *dst) {
    uint16_t addr;
    READ_WORD(reader, addr);

    return sprintf(dst, "%d", addr);
}

int effective_addr_base(uint8_t r_m, char *dst) {
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

char *effective_addr(BinFileReader *reader, char *dst, uint8_t mod, uint8_t r_m) {
    *dst++ = '[';

    if(mod == 0x00 && r_m == 0x06) {
        // Direct Address
        dst += read_16_bit_val(reader, dst);
        goto end;
    }

    dst += effective_addr_base(r_m, dst);

    if(mod == 0x00) {
        goto end;
    }

    *dst++ = ' ';
    *dst++ = '+';
    *dst++ = ' ';

    if(mod == 0x01) {
        // 8-bit displacement
        dst += read_8_bit_val(reader, dst);
    } else {
        // 16-bit displacement
        dst += read_16_bit_val(reader, dst);
    }

    end:
    *dst++ = ']';
    return dst;
}

char *resolve_rm_addr(BinFileReader *reader, char *dst, uint8_t mod, uint8_t r_m, bool w) {
    switch(mod) {
        case 0x00: case 0x01: case 0x02: {
            dst = effective_addr(reader, dst, mod, r_m);
            break;
        }
        case 0x03: {
            // Register Mode
            dst += sprintf(dst, "%s", w ? wordRegs[r_m] : byteRegs[r_m]);
            break;
        }
        default: break;
    }

    return dst;
}

char *resolve_reg(char *dst, uint8_t  reg, bool w) {
    dst += sprintf(dst, "%s", w ? wordRegs[reg] : byteRegs[reg]);
    return dst;
}

char *resolve_immediate_val(BinFileReader *reader, char *dst, bool w) {
    if(w) {
        dst += read_16_bit_val(reader, dst);
    } else {
        dst += read_8_bit_val(reader, dst);
    }

    return dst;
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

    regArg = resolve_reg(regArg, reg, w);
    r_mArg = resolve_rm_addr(reader, r_mArg, mod, r_m, w);

    *regArg++ = 0;
    *r_mArg++ = 0;

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

    r_mArg = resolve_rm_addr(reader, r_mArg, mod, r_m, w);
    immArg = resolve_immediate_val(reader, immArg, w);

    *immArg++ = 0;
    *r_mArg++ = 0;

    fprintf(out, "mov %s, %s\n", dstBuf, srcBuf);
    return Opcode_Err_OK;
}

// MOV immediate to register
Opcode_Err mov_imm_to_reg(FILE *out, BinFileReader *reader, uint8_t cmd) {
    char srcBuf[MAX_ARG_LEN], dstBuf[MAX_ARG_LEN];
    char *immArg = srcBuf, *regArg = dstBuf;

    bool w = cmd & 0x08;        // 0b00001000
    uint8_t reg = cmd & 0x07;   // 0b00000111

    regArg = resolve_reg(regArg, reg, w);
    immArg = resolve_immediate_val(reader, immArg, w);

    *immArg++ = 0;
    *regArg++ = 0;

    fprintf(out, "mov %s, %s\n", dstBuf, srcBuf);
    return Opcode_Err_OK;
}

// MOV memory to/from accumulator
Opcode_Err mov_mem_tf_acc(FILE *out, BinFileReader *reader, uint8_t cmd) {
    char addrBuf[MAX_ARG_LEN];

    bool d = (cmd & 0x02) >> 1; // 0b00000010
    bool w = cmd & 0x01;        // 0b00000001

    const char *acc = w ? wordRegs[REG_AX] : byteRegs[REG_AL];
    read_16_bit_val(reader, addrBuf);

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
        [0xA3] = mov_mem_tf_acc, // TEST is it with AL?

};

#undef MAX_ARG_LEN
#undef READ_BYTE
#undef READ_WORD
#undef READ_MOD_REG_RM
