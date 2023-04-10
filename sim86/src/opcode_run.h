#ifndef SIM86_OPCODE_SIMULATE_H
#define SIM86_OPCODE_SIMULATE_H

#include "opcode.h"
#include "memory.h"

void Opcode_run(const Opcode *opcode, Memory *memory, FILE *trace);

#endif //SIM86_OPCODE_SIMULATE_H
