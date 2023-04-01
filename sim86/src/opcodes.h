#ifndef PERFAWARE_OPCODES_H
#define PERFAWARE_OPCODES_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "bin_file_reader.h"

#define OPCODE_COUNT 256

typedef enum {
    Opcode_Err_OK = 0,
    Opcode_Err_EOF,
    Opcode_Err_FILE_ERR,
    Opcode_Err_INVALID_ARG,
    Opcode_Err_UNREACHABLE,
} Opcode_Err;

typedef Opcode_Err (*Opcode)(FILE *out, BinFileReader *reader, uint8_t cmd);

extern Opcode opcodes[OPCODE_COUNT];

#endif //PERFAWARE_OPCODES_H
