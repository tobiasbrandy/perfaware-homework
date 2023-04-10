#include "opcode_run.h"

#include <stdlib.h>
#include <assert.h>

#include "opcode_decompile.h"

static uint8_t *reg_ptr_byte(const OpcodeRegAccess *access, const Memory *memory) {
    const uint16_t *reg = &memory->registers[access->reg];
    const RegOffset offset = access->offset;

    if(access->offset == RegOffset_NONE) {
        fprintf(stderr, "Opcode register access offset is missing on byte register!\n");
        abort();
    }

    return ((uint8_t *) reg) + (offset - 1);
}

static uint16_t *reg_ptr_word(const OpcodeRegAccess *access, const Memory *memory) {
    if(access->offset != RegOffset_NONE) {
        fprintf(stderr, "Opcode register access offset is set on word register!\n");
        abort();
    }
    return (uint16_t *) &memory->registers[access->reg];
}

static uint16_t get_register(const OpcodeRegAccess *access, const Memory *memory) {
    switch(access->size) {
        case RegSize_BYTE: {
            return *reg_ptr_byte(access, memory);
        }
        case RegSize_WORD: {
            return *reg_ptr_word(access, memory);
        }
        case RegSize_NONE: {
            fprintf(stderr, "Opcode register access size missing!\n");
            abort();
        }
    }
    assert(false);
}

static inline uint32_t mem_effective_addr(const OpcodeMemAccess *access, const Memory *memory) {
    return (access->terms[0].present ? get_register(&access->terms[0].reg, memory) : 0)
           + (access->terms[0].present ? get_register(&access->terms[0].reg, memory) : 0)
           + access->displacement
           ;
}

static inline uint16_t get_memory(const OpcodeMemAccess *access, const Memory *memory) {
    return memory->ram[mem_effective_addr(access, memory)];
}

static inline uint16_t get_immediate(const OpcodeImmAccess *access) {
    return access->value;
}

static uint16_t get_arg_data(const OpcodeArg *arg, const Memory *memory) {
    switch(arg->type) {
        case OpcodeArgType_REGISTER: return get_register(&arg->reg, memory);
        case OpcodeArgType_MEMORY: return get_memory(&arg->mem, memory);
        case OpcodeArgType_IMMEDIATE:
        case OpcodeArgType_IPINC: return get_immediate(&arg->imm);
        case OpcodeArgType_NONE: {
            fprintf(stderr, "Opcode argument missing!\n");
            abort();
        }
    }
    assert(false);
}

static void set_register(const OpcodeRegAccess *access, Memory *memory, const uint16_t data, FILE *trace) {
    switch(access->size) {
        case RegSize_BYTE: {
            *reg_ptr_byte(access, memory) = data;
        } break;
        case RegSize_WORD: {
            *reg_ptr_word(access, memory) = data;
        } break;
        case RegSize_NONE: {
            fprintf(stderr, "Opcode register access size missing!\n");
            abort();
        }
    }
}

static inline void set_memory(const OpcodeMemAccess *access, Memory *memory, const uint16_t data, FILE *trace) {
    memory->ram[mem_effective_addr(access, memory)] = data;
}

static void set_arg_data(const OpcodeArg *arg, Memory *memory, const uint16_t data, FILE *trace) {
    if(trace) {
        char opArg[MAX_OP_ARG_LEN + 1];
        OpcodeArg_decompile(arg, false, opArg);
        fprintf(trace, "%s:0x%x->0x%x", opArg, get_arg_data(arg, memory), data);
    }

    switch(arg->type) {
        case OpcodeArgType_REGISTER: set_register(&arg->reg, memory, data, trace); break;
        case OpcodeArgType_MEMORY: set_memory(&arg->mem, memory, data, trace); break;
        case OpcodeArgType_IMMEDIATE:
        case OpcodeArgType_IPINC: {
            fprintf(stderr, "Invalid memory set on immediate value argument!\n");
            abort();
        }
        case OpcodeArgType_NONE: {
            fprintf(stderr, "Opcode argument missing!\n");
            abort();
        }
    }
}

/* -------------------- OPCODES --------------------------- */

typedef void (*OpcodeF)(const Opcode *opcode, Memory *memory, FILE* trace);

static void NONE(const Opcode *opcode, Memory *memory, FILE *trace) {
    fprintf(stderr, "Opcode missing!\n");
    abort();
}

static void MOV(const Opcode *opcode, Memory *memory, FILE *trace) {
    set_arg_data(&opcode->dst, memory, get_arg_data(&opcode->src, memory), trace);
}

static void ADD(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void ADC(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void SUB(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void SBB(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void CMP(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void AND(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void OR(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void XOR(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JE(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JL(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JLE(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JB(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JBE(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JP(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JO(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JS(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JNE(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JNL(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JNLE(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JNB(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JNBE(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JNP(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JNO(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JNS(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void LOOP(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void LOOPE(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void LOOPNE(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void JCXZ(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

void Opcode_run(const Opcode *opcode, Memory *memory, FILE *trace) {
    static OpcodeF ops[OpcodeType_COUNT] = {
            [OpcodeType_NONE] = NONE,

            #define OPCODE(name, ...) [OpcodeType_##name] = name,
            #define SUB_OP(...)
            #include "opcode_encoding_table.inl"
    };
    ops[opcode->type](opcode, memory, trace);
}
