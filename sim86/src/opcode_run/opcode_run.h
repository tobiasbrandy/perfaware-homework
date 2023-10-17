#ifndef SIM86_OPCODE_SIMULATE_H
#define SIM86_OPCODE_SIMULATE_H

#include "opcode/opcode.h"
#include "memory/memory.h"

void Opcode_run(const Opcode *opcode, Memory *memory, FILE *trace);

#endif //SIM86_OPCODE_SIMULATE_H
