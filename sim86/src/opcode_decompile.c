#include "opcode_decompile.h"

#include <assert.h>
#include <ctype.h>

int OpcodeType_decompile(OpcodeType type, char *dst) {
    static const char *nameTable[] = {
#define OPCODE(name, ...) [OpcodeType_##name] = #name,
#define SUB_OP(...)
#include "opcode_encoding_table.inl"
    };
    const char *name = nameTable[type];

    int i;
    for(i = 0; name[i]; ++i) {
        dst[i] = (char) tolower(name[i]);
    }
    dst[i] = 0;

    return i;
}

const char *OpcodeRegAccess_decompile(const OpcodeRegAccess *regAccess) {
    static const char *byteRegs[8] = {
            [(Register_AX << 1) | 0] = "al",
            [(Register_BX << 1) | 0] = "bl",
            [(Register_CX << 1) | 0] = "cl",
            [(Register_DX << 1) | 0] = "dl",
            [(Register_AX << 1) | 1] = "ah",
            [(Register_BX << 1) | 1] = "bh",
            [(Register_CX << 1) | 1] = "ch",
            [(Register_DX << 1) | 1] = "dh",
    };
    static const char *wordRegs[Register_COUNT] = {
            [Register_AX] = "ax",
            [Register_BX] = "bx",
            [Register_CX] = "cx",
            [Register_DX] = "dx",
            [Register_SP] = "sp",
            [Register_BP] = "bp",
            [Register_SI] = "si",
            [Register_DI] = "di",
            [Register_ES] = "es",
            [Register_CS] = "cs",
            [Register_SS] = "ss",
            [Register_DS] = "ds",
            [Register_IP] = "ip",
    };

    const bool invalid
         = (regAccess->size == RegSize_WORD && regAccess->offset != RegOffset_NONE)
        || (regAccess->size == RegSize_BYTE && regAccess->offset == RegOffset_NONE)
        ;
    if(invalid) {
        return "invalid";
    }

    if(regAccess->size == RegSize_BYTE) {
        return byteRegs[(regAccess->reg << 1) | (regAccess->offset - 1)];
    } else { // regAccess->size == RegSize_WORD
        return wordRegs[regAccess->reg];
    }
}

int OpcodeMemAccess_decompile(const OpcodeMemAccess *memAccess, char *dst) {
    const char *ogDst = dst;
    const OpcodeAddrRegTerm *terms = memAccess->terms;
    const int16_t displacement = memAccess->displacement;

    *dst++ = '[';

    if(terms[0].present) {
        dst += sprintf(dst, "%s", OpcodeRegAccess_decompile(&memAccess->terms[0].reg));
    }

    if(terms[1].present) {
        dst += sprintf(dst, " + %s", OpcodeRegAccess_decompile(&memAccess->terms[1].reg));
    }

    if(displacement) {
        if(!terms[0].present && !terms[1].present) {
            dst += sprintf(dst, "%d", displacement);
        } else if(displacement > 0) {
            dst += sprintf(dst, " + %d", displacement);
        } else {
            dst += sprintf(dst, " - %d", -displacement);
        }
    }

    *dst++ = ']';
    *dst = 0;

    return (int) (dst - ogDst);
}

const char *RegSize_decompile(RegSize regSize) {
    switch(regSize) {
        case RegSize_BYTE: return "byte";
        case RegSize_WORD: return "word";
    }
    return "";
}

int OpcodeImmAccess_decompile(const OpcodeImmAccess *immAccess, bool explicitSize, char *dst) {
    const char *size = explicitSize ? RegSize_decompile(immAccess->size) : "";
    const char *ws = explicitSize ? " " : "";
    const int value = immAccess->size == RegSize_BYTE ? (uint8_t) immAccess->value : (uint16_t) immAccess->value;
    return sprintf(dst, "%s%s%d", size, ws, value);
}

int OpcodeIpincAccess_decompile(const OpcodeImmAccess *ipincAccess, char *dst) {
    // ipinc = increment after jmp instruction
    const int ipinc = ipincAccess->value + 2;

    if(ipinc >= 0) {
        return sprintf(dst, "$+%d", ipinc);
    } else {
        return sprintf(dst, "$%d", ipinc);
    }
}

int OpcodeArg_decompile(const OpcodeArg *arg, bool explicitSize, char *dst) {
    switch(arg->type) {
        case OpcodeArgType_NONE: return 0;
        case OpcodeArgType_REGISTER: return sprintf(dst, "%s", OpcodeRegAccess_decompile(&arg->reg));
        case OpcodeArgType_MEMORY: return OpcodeMemAccess_decompile(&arg->mem, dst);
        case OpcodeArgType_IMMEDIATE: return OpcodeImmAccess_decompile(&arg->imm, explicitSize, dst);
        case OpcodeArgType_IPINC: return OpcodeIpincAccess_decompile(&arg->ipinc, dst);
        default: assert(false);
    }
}

int Opcode_decompile(const Opcode *opcode, char *dst) {
    char *ogDst = dst;

    const bool explicitSize =
            (opcode->dst.type == OpcodeArgType_MEMORY && opcode->src.type == OpcodeArgType_IMMEDIATE)
            || (opcode->src.type == OpcodeArgType_MEMORY && opcode->dst.type == OpcodeArgType_IMMEDIATE)
    ;

    dst += OpcodeType_decompile(opcode->type, dst);

    if(opcode->dst.type != OpcodeArgType_NONE) {
        *dst++ = ' ';
        dst += OpcodeArg_decompile(&opcode->dst, explicitSize, dst);
    }

    if(opcode->src.type != OpcodeArgType_NONE) {
        *dst++ = ',';
        *dst++ = ' ';
        dst += OpcodeArg_decompile(&opcode->src, explicitSize, dst);
    }

    *dst = 0;

    return (int) (dst - ogDst);
}

void Opcode_decompile_to_file(const Opcode *opcode, FILE *out) {
    char buf[MAX_OP_LEN + 1];
    Opcode_decompile(opcode, buf);
    fputs(buf, out);
}

#undef MAX_ARG_LEN
