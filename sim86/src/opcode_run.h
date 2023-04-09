#ifndef SIM86_OPCODE_SIMULATE_H
#define SIM86_OPCODE_SIMULATE_H

#include "opcode.h"
#include "memory.h"

void simulate_run(const Opcode *opcode, Memory *memory);

#endif //SIM86_OPCODE_SIMULATE_H
