#ifndef SIM86_MEMORY_H
#define SIM86_MEMORY_H

#include <stdint.h>
#include <stdio.h>

#include "opcode.h"

#define RAM_SIZE 0x100000 // 1 MB
#define SEGMENT_SIZE 0x10000

#define RAM_LOW_RESERVED 0x80
#define RAM_HIGH_RESERVED 0xFFFF0

#define FLAG_COUNT 9

typedef struct {
    bool
    overflow, direction, interrupt, trap,
    sign, zero, auxCarry, parity,
    carry;
} Flags;

typedef struct {
    uint8_t *ram;
    uint8_t *codeEnd; // Keep track of when to finish
    uint16_t registers[Register_COUNT];
    Flags flags;
} Memory;

Memory Memory_create(void);

uint8_t *Memory_segment_ptr(const Memory *mem, Register segmentReg);

const uint8_t *Memory_code_ptr(const Memory *mem);

uint8_t *Memory_addr_ptr(const Memory *mem, Register segmentReg, uint16_t addr);

bool Memory_code_ended(const Memory *mem);

bool Memory_load_code(Memory *mem, FILE *code);

int Flags_serialize(const Flags *flags, char *dst);

#endif //SIM86_MEMORY_H
