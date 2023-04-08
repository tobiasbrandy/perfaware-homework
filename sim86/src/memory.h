#ifndef SIM86_MEMORY_H
#define SIM86_MEMORY_H

#include <stdint.h>
#include <stdio.h>

#define RAM_SIZE (1*1024*1024)

typedef struct {
    uint8_t *ram;
} Memory;

Memory Memory_create(void);

int Memory_load_code(Memory *mem, FILE *code);

#endif //SIM86_MEMORY_H
