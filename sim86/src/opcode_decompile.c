#include "opcode_decompile.h"

#include <assert.h>
#include <ctype.h>

int decompile_opcode_type(char *dst, const OpcodeType type) {
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

int decompile_opcode_reg_access(char *dst, const OpcodeRegAccess *regAccess) {
    static const char *regs[Register_COUNT] = {
            [Register_AX] = "a",
            [Register_BX] = "b",
            [Register_CX] = "c",
            [Register_DX] = "d",
            [Register_SP] = "sp",
            [Register_BP] = "bp",
            [Register_SI] = "si",
            [Register_DI] = "di",
    };
    const char *reg = regs[regAccess->reg];

    if(regAccess->size == RegSize_BYTE) {
        return sprintf(dst, "%s%c", reg, regAccess->offset == RegOffset_LOW ? 'l' : 'h');
    } else if(reg[1] == 0) {
        return sprintf(dst, "%sx", reg);
    } else {
        return sprintf(dst, "%s", reg);
    }
}

int decompile_opcode_mem_access(char *dst, const OpcodeMemAccess *memAccess) {
    const char *ogDst = dst;
    const OpcodeAddrRegTerm *terms = memAccess->terms;
    const int16_t displacement = memAccess->displacement;

    *dst++ = '[';

    if(terms[0].present) {
        dst += decompile_opcode_reg_access(dst, &memAccess->terms[0].reg);
    }

    if(terms[1].present) {
        dst += sprintf(dst, " + ");
        dst += decompile_opcode_reg_access(dst, &memAccess->terms[1].reg);
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

const char *decompile_reg_size(const RegSize regSize) {
    switch(regSize) {
        case RegSize_NONE: return "";
        case RegSize_BYTE: return "byte";
        case RegSize_WORD: return "word";
    }
    return "";
}

int decompile_opcode_imm_access(char *dst, const OpcodeImmAccess *immAccess, const bool explicitSize) {
    const char *size = explicitSize ? decompile_reg_size(immAccess->size) : "";
    const char *ws = explicitSize ? " " : "";
    return sprintf(dst, "%s%s%d", size, ws, immAccess->value);
}

int decompile_opcode_ipinc_access(char *dst, const OpcodeImmAccess *ipincAccess) {
    // ipinc = increment after jmp instruction
    const int ipinc = ipincAccess->value + 2;

    if(ipinc >= 0) {
        return sprintf(dst, "$+%d", ipinc);
    } else {
        return sprintf(dst, "$%d", ipinc);
    }
}

int decompile_opcode_arg(char *dst, const OpcodeArg *arg, const bool explicitSize) {
    switch(arg->type) {
        case OpcodeArgType_NONE: return 0;
        case OpcodeArgType_REGISTER: return decompile_opcode_reg_access(dst, &arg->reg);
        case OpcodeArgType_MEMORY: return decompile_opcode_mem_access(dst, &arg->mem);
        case OpcodeArgType_IMMEDIATE: return decompile_opcode_imm_access(dst, &arg->imm, explicitSize);
        case OpcodeArgType_IPINC: return decompile_opcode_ipinc_access(dst, &arg->ipinc);
        default: assert(false);
    }
}

int decompile_opcode(char *dst, const Opcode *opcode) {
    char *ogDst = dst;

    const bool explicitSize =
            (opcode->dst.type == OpcodeArgType_MEMORY && opcode->src.type == OpcodeArgType_IMMEDIATE)
            || (opcode->src.type == OpcodeArgType_MEMORY && opcode->dst.type == OpcodeArgType_IMMEDIATE)
    ;

    dst += decompile_opcode_type(dst, opcode->type);

    if(opcode->dst.type != OpcodeArgType_NONE) {
        *dst++ = ' ';
        dst += decompile_opcode_arg(dst, &opcode->dst, explicitSize);
    }

    if(opcode->src.type != OpcodeArgType_NONE) {
        *dst++ = ',';
        *dst++ = ' ';
        dst += decompile_opcode_arg(dst, &opcode->src, explicitSize);
    }

    *dst = 0;

    return (int) (dst - ogDst);
}

void decompile_opcode_to_file(const Opcode *opcode, FILE *out) {
    char buf[MAX_OP_LEN + 1];
    decompile_opcode(buf, opcode);
    fputs(buf, out);
}

#undef MAX_ARG_LEN
