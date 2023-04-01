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
    uint8_t *tmp;\
    if((tmp = BinFileReader_read_bytes(reader, 1)) == NULL) {\
        return read_error_code(reader);\
    }\
    out = *tmp;\
    } while(0)

#define READ_WORD(reader, out) do { \
    uint8_t *tmp;\
    if((tmp = BinFileReader_read_bytes(reader, 2)) == NULL) {\
        return read_error_code(reader);\
    }\
    out = (tmp[1] << 8) + tmp[0];\
    } while(0)

Opcode_Err read_error_code(BinFileReader *reader) {
    if(BinFileReader_status(reader) == BinFileReader_Status_ERROR) {
        return Opcode_Err_FILE_ERR;
    }
    return Opcode_Err_EOF;
}

int displacement_8(BinFileReader *reader, char *dst) {
    uint8_t addr;
    READ_BYTE(reader, addr);

    return sprintf(dst, "%d", addr);
}

int displacement_16(BinFileReader *reader, char *dst) {
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
        dst += displacement_16(reader, dst);
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
        // 8-bit
        dst += displacement_8(reader, dst);
    } else {
        // 16-bit
        dst += displacement_16(reader, dst);
    }

    end:
    *dst++ = ']';
    return dst;
}

// MOV register/memory to/from register
Opcode_Err mov_rm_tf_reg(FILE *out, BinFileReader *reader, uint8_t cmd) {
    char srcBuf[MAX_ARG_LEN], dstBuf[MAX_ARG_LEN];

    bool d = (cmd & 0x02) >> 1; // 0b00000010
    bool w = cmd & 0x01;        // 0b00000001

    char *regArg = d ? dstBuf : srcBuf;
    char *r_mArg = !d ? dstBuf : srcBuf;

    uint8_t cmd2;
    READ_BYTE(reader, cmd2);

    uint8_t mod = (cmd2 & 0xC0) >> 6; // 0b11000000
    uint8_t reg = (cmd2 & 0x38) >> 3; // 0b00111000
    uint8_t r_m = cmd2 & 0x07;        // 0b00000111

    regArg += sprintf(regArg, "%s", w ? wordRegs[reg] : byteRegs[reg]);

    switch(mod) {
        case 0x00:
        case 0x01:
        case 0x02: {
            r_mArg = effective_addr(reader, r_mArg, mod, r_m);
            break;
        }
        case 0x03: {
            // Register Mode
            regArg += sprintf(regArg, "%s", w ? wordRegs[r_m] : byteRegs[r_m]);
            break;
        }
        default: {
            return Opcode_Err_UNREACHABLE;
        }
    }

    *regArg++ = 0;
    *r_mArg++ = 0;

    fprintf(out, "mov %s, %s\n", dstBuf, srcBuf);
    return Opcode_Err_OK;
}

Opcode opcodes[OPCODE_COUNT] = {
        // 0b100010XX: MOV register/memory to/from register
        [0x88] = mov_rm_tf_reg,
        [0x89] = mov_rm_tf_reg,
        [0x8A] = mov_rm_tf_reg,
        [0x8B] = mov_rm_tf_reg,
};

#undef READ_BYTE
