#include "memory.h"

//#define INIT_SEGMENT_POS(n) (RAM_LOW_RESERVED + n*(SEGMENT_SIZE >> 4))
#define INIT_SEGMENT_POS(n) 0 // TODO: Pongo esto para que salga bien el output de trace
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
                    [Register_ES] = INIT_SEGMENT_POS(0),
                    [Register_CS] = INIT_SEGMENT_POS(1),
                    [Register_SS] = INIT_SEGMENT_POS(2),
                    [Register_DS] = INIT_SEGMENT_POS(3),
                    [Register_IP] = 0,
            },
            .flags = {0},
    };
    return ret;
}

inline uint8_t *Memory_segment_ptr(const Memory *mem, const Register segmentReg) {
    return &mem->ram[segmentReg << 4];
}

inline const uint8_t *Memory_code_ptr(const Memory *mem) {
    return Memory_segment_ptr(mem, Register_CS) + mem->registers[Register_IP];
}

inline uint8_t *Memory_addr_ptr(const Memory *mem, const Register segmentReg, const uint16_t addr) {
    return Memory_segment_ptr(mem, segmentReg) + addr;
}

inline bool Memory_code_ended(const Memory *mem) {
    return Memory_code_ptr(mem) == mem->codeEnd;
}

bool Memory_load_code(Memory *mem, FILE *codeSrc) {
    uint8_t *codeSegment = Memory_segment_ptr(mem, Register_CS);

    int codeLen = (int) fread(codeSegment, 1, SEGMENT_SIZE, codeSrc);
    if(codeLen < SEGMENT_SIZE && ferror(codeSrc)) {
        return false;
    }

    mem->codeEnd = codeSegment + codeLen;
    return true;
}

int Flags_serialize(const Flags *flags, char *dst) {
    char *ogDst = dst;
    if(flags->carry) *dst++ = 'C';
    if(flags->parity) *dst++ = 'P';
    if(flags->auxCarry) *dst++ = 'A';
    if(flags->zero) *dst++ = 'Z';
    if(flags->sign) *dst++ = 'S';
    if(flags->overflow) *dst++ = 'O';
    if(flags->trap) *dst++ = 'T';
    if(flags->interrupt) *dst++ = 'I';
    if(flags->direction) *dst++ = 'D';
    *dst = 0;
    return (int) (dst - ogDst);
}

#undef INIT_SEGMENT_POS
#undef MEM_MASK
