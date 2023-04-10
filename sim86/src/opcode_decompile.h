
#ifndef SIM86_OPCODE_DECOMPILE_H
#define SIM86_OPCODE_DECOMPILE_H

#include <stdio.h>

#include "opcode.h"

#define MAX_OP_NAME_LEN 10 // Example: SEGMENT
#define MAX_OP_ARG_LEN 30  // Example: `word [bp + di - 10044]\0`
#define MAX_OP_LEN (MAX_OP_NAME_LEN + 2*MAX_OP_ARG_LEN)

int OpcodeType_decompile(OpcodeType type, char *dst);

int OpcodeRegAccess_decompile(const OpcodeRegAccess *regAccess, char *dst);

int OpcodeMemAccess_decompile(const OpcodeMemAccess *memAccess, char *dst);

const char *RegSize_decompile(RegSize regSize);

int OpcodeImmAccess_decompile(const OpcodeImmAccess *immAccess, bool explicitSize, char *dst);

int OpcodeIpincAccess_decompile(const OpcodeImmAccess *ipincAccess, char *dst);

int OpcodeArg_decompile(const OpcodeArg *arg, bool explicitSize, char *dst);

int Opcode_decompile(const Opcode *opcode, char *dst);

void Opcode_decompile_to_file(const Opcode *opcode, FILE *out);

#endif //SIM86_OPCODE_DECOMPILE_H
