
#ifndef SIM86_OPCODE_DECOMPILE_H
#define SIM86_OPCODE_DECOMPILE_H

#include <stdio.h>

#include "opcode.h"

#define MAX_OP_NAME_LEN 10 // Example: SEGMENT
#define MAX_OP_ARG_LEN 30  // Example: `word [bp + di - 10044]\0`
#define MAX_OP_LEN (MAX_OP_NAME_LEN + 2*MAX_OP_ARG_LEN)

int decompile_opcode_type(char *dst, OpcodeType type);

int decompile_opcode_reg_access(char *dst, const OpcodeRegAccess *regAccess);

int decompile_opcode_mem_access(char *dst, const OpcodeMemAccess *memAccess);

const char *decompile_reg_size(RegSize regSize);

int decompile_opcode_imm_access(char *dst, const OpcodeImmAccess *immAccess, bool explicitSize);

int decompile_opcode_ipinc_access(char *dst, const OpcodeImmAccess *ipincAccess);

int decompile_opcode_arg(char *dst, const OpcodeArg *arg, bool explicitSize);

int decompile_opcode(char *dst, const Opcode *opcode);

void decompile_opcode_to_file(const Opcode *opcode, FILE *out);

#endif //SIM86_OPCODE_DECOMPILE_H
