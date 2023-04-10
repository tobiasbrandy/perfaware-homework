#include "memory.h"

#define INIT_SEGMENT_POS(n) (RAM_LOW_RESERVED + n*(SEGMENT_SIZE >> 4))
#define MEM_MASK (RAM_SIZE-1)

static uint8_t ram[RAM_SIZE];

Memory Memory_create(void) {
    Memory ret = {
            .ram = ram,
            .codeEnd = NULL,
            .registers = {
                    [Register_AX] = 0,
                    [Register_BX] = 0,
                    [Register_CX] = 0,
                    [Register_DX] = 0,
                    [Register_SP] = 0,
                    [Register_BP] = 0,
                    [Register_SI] = 0,
                    [Register_DI] = 0,
                    [Register_CS] = INIT_SEGMENT_POS(0),
                    [Register_SS] = INIT_SEGMENT_POS(1),
                    [Register_DS] = INIT_SEGMENT_POS(2),
                    [Register_ES] = INIT_SEGMENT_POS(3),
                    [Register_IP] = 0,
            },
            .flags = {0},
    };
    return ret;
}

static inline uint8_t *get_segment(const Memory *mem, const Register segmentReg) {
    return &mem->ram[segmentReg << 4];
}

const uint8_t *Memory_code_ptr(const Memory *mem) {
    return get_segment(mem, Register_CS) + mem->registers[Register_IP];
}

bool Memory_load_code(Memory *mem, FILE *codeSrc) {
    uint8_t *codeSegment = get_segment(mem, Register_CS);

    int codeLen = (int) fread(codeSegment, 1, SEGMENT_SIZE, codeSrc);
    if(codeLen < SEGMENT_SIZE && ferror(codeSrc)) {
        return false;
    }

    mem->codeEnd = codeSegment + codeLen;
    return true;
}

inline bool Memory_code_ended(const Memory *mem) {
    return Memory_code_ptr(mem) == mem->codeEnd;
}

#undef INIT_SEGMENT_POS
#undef MEM_MASK
