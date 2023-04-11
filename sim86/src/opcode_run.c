#include "opcode_run.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

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
    // TODO: Make segment selection more robust, depending on opcode
    const OpcodeAddrRegTerm lTerm = access->terms[0];
    const Register segmentReg = lTerm.present && lTerm.reg.reg == Register_BP ? Register_SS : Register_DS;
    const uint8_t *addrPtr = Memory_addr_ptr(memory, segmentReg, mem_effective_addr(access, memory));
    switch(access->size) {
        case RegSize_BYTE: return *addrPtr;
        case RegSize_WORD: return (addrPtr[1] << 8) | addrPtr[0]; // Little endian
    }
    assert(false);
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

static void set_register(const OpcodeRegAccess *access, Memory *memory, const uint16_t data) {
    switch(access->size) {
        case RegSize_BYTE: {
            *reg_ptr_byte(access, memory) = data;
        } break;
        case RegSize_WORD: {
            *reg_ptr_word(access, memory) = data;
        } break;
    }
}

static inline void set_memory(const OpcodeMemAccess *access, Memory *memory, const uint16_t data) {
    // TODO: Make segment selection more robust, depending on opcode
    const OpcodeAddrRegTerm lTerm = access->terms[0];
    const Register segmentReg = lTerm.present && lTerm.reg.reg == Register_BP ? Register_SS : Register_DS;
    uint8_t *addrPtr = Memory_addr_ptr(memory, segmentReg, mem_effective_addr(access, memory));
    switch(access->size) {
        case RegSize_BYTE: *addrPtr = data;
        case RegSize_WORD: {
            // Little endian
            addrPtr[0] = data;
            addrPtr[1] = data >> 8;
        }
    }
}

static void set_arg_data(const OpcodeArg *arg, Memory *memory, const uint16_t data, FILE *trace) {
    // Trace setup
    OpcodeArg traceArg = *arg;
    uint16_t ogArgData;
    if(trace) {
        if(arg->type == OpcodeArgType_REGISTER) {
            // We always trace the full register
            traceArg.reg.size = RegSize_WORD;
            traceArg.reg.offset = RegOffset_NONE;
        }

        ogArgData = get_arg_data(&traceArg, memory);
    }

    switch(arg->type) {
        case OpcodeArgType_REGISTER: set_register(&arg->reg, memory, data); break;
        case OpcodeArgType_MEMORY: set_memory(&arg->mem, memory, data); break;
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

    if(trace) {
        char opArg[MAX_OP_ARG_LEN + 1];
        OpcodeArg_decompile(&traceArg, false, opArg);
        fprintf(trace, " %s:0x%x->0x%x", opArg, ogArgData, get_arg_data(&traceArg, memory));
    }
}

/* ------------------------- FLAGS ---------------------- */

static inline bool set_add_carry(const RegSize size, const uint16_t a, const uint16_t b) {
    return b > RegSize_max(size) - a;
}

static inline bool set_sub_carry(const uint16_t a, const uint16_t b) {
    return a < b;
}

static inline bool set_add_overflow(const RegSize size, const uint16_t a, const uint16_t b) {
    return set_add_carry(size, a << 1, b << 1) ^ set_add_carry(size, a, b);
}

static inline bool set_sub_overflow(const uint16_t a, const uint16_t b) {
    return set_sub_carry(a << 1, b << 1) ^ set_sub_carry(a, b);
}

static inline bool set_add_aux_carry(const uint16_t a, const uint16_t b) {
    return (b & 0xF) > 0xF - (a & 0xF);
}

static inline bool set_sub_aux_carry(const uint16_t a, const uint16_t b) {
    return set_sub_carry(a & 0xF, b & 0xF);
}

static inline bool set_sign(const RegSize size, const uint16_t result) {
    return ((RegSize_max(size) + 1) >> 1) & result;
}

static inline bool set_zero(const uint16_t result) {
    return result == 0;
}

static inline bool set_parity(const uint8_t result) {
    uint8_t y = result ^ (result >> 1);
    y = y ^ (y >> 2);
    y = y ^ (y >> 4);
    return !(y & 1);
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
    const uint16_t l = get_arg_data(&opcode->dst, memory);
    const uint16_t r = get_arg_data(&opcode->src, memory);
    const uint16_t result = l + r;

    set_arg_data(&opcode->dst, memory, result, trace);

    const RegSize size = OpcodeArg_size(&opcode->dst);
    memory->flags.overflow = set_add_overflow(size, l, r);
    memory->flags.sign = set_sign(size, result);
    memory->flags.zero = set_zero(result);
    memory->flags.auxCarry = set_add_aux_carry(l, r);
    memory->flags.parity = set_parity(result);
    memory->flags.carry = set_add_carry(size, l, r);
}

static void ADC(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void SUB(const Opcode *opcode, Memory *memory, FILE *trace) {
    const uint16_t l = get_arg_data(&opcode->dst, memory);
    const uint16_t r = get_arg_data(&opcode->src, memory);
    const uint16_t result = l - r;

    set_arg_data(&opcode->dst, memory, result, trace);

    const RegSize size = OpcodeArg_size(&opcode->dst);
    memory->flags.overflow = set_sub_overflow(l, r);
    memory->flags.sign = set_sign(size, result);
    memory->flags.zero = set_zero(result);
    memory->flags.auxCarry = set_sub_aux_carry(l, r);
    memory->flags.parity = set_parity(result);
    memory->flags.carry = set_sub_carry(l, r);
}

static void SBB(const Opcode *opcode, Memory *memory, FILE *trace) {
    // TODO
}

static void CMP(const Opcode *opcode, Memory *memory, FILE *trace) {
    const uint16_t l = get_arg_data(&opcode->dst, memory);
    const uint16_t r = get_arg_data(&opcode->src, memory);
    const uint16_t result = l - r;

    const RegSize size = OpcodeArg_size(&opcode->dst);
    memory->flags.overflow = set_sub_overflow(l, r);
    memory->flags.sign = set_sign(size, result);
    memory->flags.zero = set_zero(result);
    memory->flags.auxCarry = set_sub_aux_carry(l, r);
    memory->flags.parity = set_parity(result);
    memory->flags.carry = set_sub_carry(l, r);
}

static void AND(const Opcode *opcode, Memory *memory, FILE *trace) {
    const uint16_t l = get_arg_data(&opcode->dst, memory);
    const uint16_t r = get_arg_data(&opcode->src, memory);
    const uint16_t result = l & r;

    set_arg_data(&opcode->dst, memory, result, trace);

    const RegSize size = OpcodeArg_size(&opcode->dst);
    memory->flags.overflow = 0;
    memory->flags.sign = set_sign(size, result);
    memory->flags.zero = set_zero(result);
    memory->flags.parity = set_parity(result);
    memory->flags.carry = 0;
}

static void OR(const Opcode *opcode, Memory *memory, FILE *trace) {
    const uint16_t l = get_arg_data(&opcode->dst, memory);
    const uint16_t r = get_arg_data(&opcode->src, memory);
    const uint16_t result = l | r;

    set_arg_data(&opcode->dst, memory, result, trace);

    const RegSize size = OpcodeArg_size(&opcode->dst);
    memory->flags.overflow = 0;
    memory->flags.sign = set_sign(size, result);
    memory->flags.zero = set_zero(result);
    memory->flags.parity = set_parity(result);
    memory->flags.carry = 0;
}

static void XOR(const Opcode *opcode, Memory *memory, FILE *trace) {
    const uint16_t l = get_arg_data(&opcode->dst, memory);
    const uint16_t r = get_arg_data(&opcode->src, memory);
    const uint16_t result = l ^ r;

    set_arg_data(&opcode->dst, memory, result, trace);

    const RegSize size = OpcodeArg_size(&opcode->dst);
    memory->flags.overflow = 0;
    memory->flags.sign = set_sign(size, result);
    memory->flags.zero = set_zero(result);
    memory->flags.parity = set_parity(result);
    memory->flags.carry = 0;
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

    const uint16_t ogIp = memory->registers[Register_IP];
    const Flags ogFlags = memory->flags;

    // Advance IP
    memory->registers[Register_IP] += opcode->len;

    // Run opcode
    ops[opcode->type](opcode, memory, trace);

    if(trace) {
        // Trace IP
        const uint16_t ip = memory->registers[Register_IP];
        if(ogIp != ip) {
            fprintf(trace, " ip:0x%x->0x%x", ogIp, ip);
        }

        // Trace flags
        char ogFlagsStr[FLAG_COUNT + 1];
        Flags_serialize(&ogFlags, ogFlagsStr);

        char flagsStr[FLAG_COUNT + 1];
        Flags_serialize(&memory->flags, flagsStr);

        if(strcmp(ogFlagsStr, flagsStr) != 0) {
            fprintf(trace, " flags:%s->%s", ogFlagsStr, flagsStr);
        }
    }
}
