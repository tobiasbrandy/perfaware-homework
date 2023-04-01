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

#define READ_BYTE(reader, out) do { \
    uint8_t *tmp;\
    if((tmp = BinFileReader_read_bytes(reader, 1)) == NULL) {\
        return read_error_code(reader);\
    }\
    out = *tmp;\
    } while(0)

Opcode_Err read_error_code(BinFileReader *reader) {
    if(BinFileReader_status(reader) == BinFileReader_Status_ERROR) {
        return Opcode_Err_FILE_ERR;
    }
    return Opcode_Err_EOF;
}

// MOV register/memory to/from register
Opcode_Err mov_rm_tf_reg(FILE *out, BinFileReader *reader, uint8_t cmd) {
    bool d = (cmd & 0x02) >> 1; // 0b00000010
    bool w = cmd & 0x01;        // 0b00000001

    uint8_t cmd2;
    READ_BYTE(reader, cmd2);

    uint8_t mod = (cmd2 & 0xC0) >> 6; // 0b11000000
    uint8_t reg = (cmd2 & 0x38) >> 3; // 0b00111000
    uint8_t r_m = cmd2 & 0x07;        // 0b00000111

    const char *src_dst[2];
    src_dst[d] = w ? wordRegs[reg] : byteRegs[reg];

    if(mod == 0x03) {
        // Register Mode
        src_dst[!d] = w ? wordRegs[r_m] : byteRegs[r_m];
    } else {
        // We still don't support other mod options
        return Opcode_Err_INVALID_ARG;
    }

    fprintf(out, "mov %s, %s\n", src_dst[1], src_dst[0]);
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
