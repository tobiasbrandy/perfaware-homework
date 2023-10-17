#include <assert.h>
#include "opcode.h"

RegSize OpcodeArg_size(const OpcodeArg *arg) {
    switch(arg->type) {
        case OpcodeArgType_NONE: assert(false);
        case OpcodeArgType_REGISTER: return arg->reg.size;
        case OpcodeArgType_MEMORY: return arg->mem.size;
        case OpcodeArgType_IMMEDIATE: return arg->imm.size;
        case OpcodeArgType_IPINC: return arg->ipinc.size;
    }
    assert(false);
}

inline int RegSize_max(const RegSize size) {
    switch(size) {
        case RegSize_BYTE: return UINT8_MAX;
        case RegSize_WORD: return UINT16_MAX;
    }
    assert(false);
}
