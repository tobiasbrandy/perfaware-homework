#include "memory.h"

static uint8_t ram[RAM_SIZE];

Memory Memory_create(void) {
    Memory ret = {
            .ram = ram,
    };
    return ret;
}

int Memory_load_code(Memory *mem, FILE *code) {
    int ret = (int) fread(mem->ram, 1, RAM_SIZE, code);
    if(ret < RAM_SIZE && ferror(code)) {
        return -1;
    }
    return ret;
}
