
#ifndef SIM86_OPCODE_DECOMPILE_H
#define SIM86_OPCODE_DECOMPILE_H

#include <stdio.h>

#include "opcode.h"

void decompile_opcode(const Opcode *opcode, FILE *out);

#endif //SIM86_OPCODE_DECOMPILE_H
