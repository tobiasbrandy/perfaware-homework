/*
 * Reference: https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf
 *
 * s = 0: No sign extension.
 * s = 1: Sign extend 8-bit immediate data to 16 bits if w = 1.
 *
 * w = 0: Instruction operates in byte data.
 * w = 1: Instruction operates in word data.
 *
 * d = 0: Instruction source is specified in REG field.
 * d = 1: Instruction destination is specified in REG field.
 *
 * mod = 00: Memory Mode, no displacement follows (expect R/M = 110, then 16 bit displacement follows).
 * mod = 01: Memory Mode, 8-bit displacement follows.
 * mod = 10: Memory Mode, 16-bit displacement follows.
 * mod = 11: Register Mode (no displacement).
 */
#include "opcode_encoding.h"

#include <assert.h>

typedef struct {
    const uint8_t *code, *end;
    uint8_t curr;
    uint8_t len;
} CodeReader;

static CodeReader CodeReader_new(const uint8_t code[], const uint8_t end[]) {
    CodeReader ret = {
      .code = code,
      .end = end,
      .curr = 0,
      .len = 0,
    };
    return ret;
}

typedef enum {
    CodeReaderErr_OK = 0,
    CodeReaderErr_END,          // No more code available to satisfy advance request
    CodeReaderErr_UNALIGNED,    // Advance request is not 8bit aligned (spans multiple bytes)
    CodeReaderErr_INVALID,      // Erroneous input
} CodeReaderErr;

inline static uint8_t right_mask(const uint8_t n) {
    return (1 << n) - 1;
}

static CodeReaderErr CodeReader_get_bits_and_advance(CodeReader *reader, const uint8_t n, uint8_t *out) {
    *out = 0;
    if(n == 0) {
        return CodeReaderErr_OK;
    }
    if(n > 8) {
        return CodeReaderErr_INVALID; // Max is 8
    }

    uint8_t len = reader->len;
    if(len > 0 && n > len) {
        return CodeReaderErr_UNALIGNED;
    }

    if(len == 0) {
        // Advance
        if(reader->code == reader->end) {
            return CodeReaderErr_END;
        }

        reader->curr = *reader->code;
        reader->code++;
        reader->len = 8;
        len = reader->len;
    }

    *out = (reader->curr & right_mask(len)) >> (len - n);
    reader->len -= n;

    return CodeReaderErr_OK;
}

static CodeReaderErr CodeReader_get_bytes_and_advance(CodeReader *reader, const uint8_t n, int16_t *out) {
    *out = 0;
    if(n == 0) {
        return CodeReaderErr_OK;
    }
    if(n > 2) {
        return CodeReaderErr_INVALID; // Max is 2
    }

    if(reader->len != 0) {
        return CodeReaderErr_UNALIGNED;
    }

    const uint8_t *data = reader->code;

    if(reader->code + n > reader->end) {
        return CodeReaderErr_END;
    }
    reader->code += n;

    if(n == 1) {
        *out = (int16_t) ((int8_t) *data);
    } else { // n == 2
        *out = (int16_t) ((data[1] << 8) | data[0]);
    }

    return CodeReaderErr_OK;
}

static OpcodeRegAccess resolve_reg_access(const uint8_t reg, const bool w) {
    static const OpcodeRegAccess regTable[][2] = {
            [0] = {{Register_AX, RegSize_BYTE, RegOffset_LOW }, {Register_AX, RegSize_WORD, RegOffset_NONE}},
            [1] = {{Register_CX, RegSize_BYTE, RegOffset_LOW }, {Register_CX, RegSize_WORD, RegOffset_NONE}},
            [2] = {{Register_DX, RegSize_BYTE, RegOffset_LOW }, {Register_DX, RegSize_WORD, RegOffset_NONE}},
            [3] = {{Register_BX, RegSize_BYTE, RegOffset_LOW }, {Register_BX, RegSize_WORD, RegOffset_NONE}},
            [4] = {{Register_AX, RegSize_BYTE, RegOffset_HIGH}, {Register_SP, RegSize_WORD, RegOffset_NONE}},
            [5] = {{Register_CX, RegSize_BYTE, RegOffset_HIGH}, {Register_BP, RegSize_WORD, RegOffset_NONE}},
            [6] = {{Register_DX, RegSize_BYTE, RegOffset_HIGH}, {Register_SI, RegSize_WORD, RegOffset_NONE}},
            [7] = {{Register_BX, RegSize_BYTE, RegOffset_HIGH}, {Register_DI, RegSize_WORD, RegOffset_NONE}},
    };
    return regTable[reg][w];
}

static OpcodeMemAccess resolve_mem_access(const uint8_t rm, const int16_t displacement, bool directAccess) {
    static const OpcodeAddrRegTerm rmToTerms[][2] = {
            [0] = {{{Register_BX, 2, 0}, true}, {{Register_SI, 2, 0}, true}},
            [1] = {{{Register_BX, 2, 0}, true}, {{Register_DI, 2, 0}, true}},
            [2] = {{{Register_BP, 2, 0}, true}, {{Register_SI, 2, 0}, true}},
            [3] = {{{Register_BP, 2, 0}, true}, {{Register_DI, 2, 0}, true}},
            [4] = {{{Register_SI, 2, 0}, true}, {{0,0,0}, false}},
            [5] = {{{Register_DI, 2, 0}, true}, {{0,0,0}, false}},
            [6] = {{{Register_BP, 2, 0}, true}, {{0,0,0}, false}},
            [7] = {{{Register_BX, 2, 0}, true}, {{0,0,0}, false}},
    };

    if(directAccess) {
        const OpcodeMemAccess ret = {
                .terms = {{{0,0,0}, false}, {{0,0,0}, false}},
                .displacement = displacement,
        };
        return ret;
    } else {
        const OpcodeMemAccess ret = {
                .terms = {rmToTerms[rm][0], rmToTerms[rm][1]},
                .displacement = displacement,
        };
        return ret;
    }
}

static OpcodeDecodeErr decode_fields(bool hasField[], uint8_t fields[], const OpcodeEncField encFields[], CodeReader *reader) {
    for(int fieldIdx = 0; encFields[fieldIdx].type != OpcodeEncFieldType_END; fieldIdx++) {
        if(fieldIdx >= MAX_ENC_FIELDS) {
            return OpcodeDecodeErr_INVALID; // Missing end field
        }

        const OpcodeEncField field = encFields[fieldIdx];
        const OpcodeEncFieldType fieldType = field.type;
        hasField[fieldType] = true;

        if(field.length == 0) {
            fields[fieldType] = field.value;
            continue;
        }

        uint8_t value;
        const CodeReaderErr err = CodeReader_get_bits_and_advance(reader, field.length, &value);
        if(err) {
            switch(err) {
                case CodeReaderErr_OK: break;
                case CodeReaderErr_END: return OpcodeDecodeErr_END;
                case CodeReaderErr_UNALIGNED: return OpcodeDecodeErr_INVALID; // Fields are cross byte
                case CodeReaderErr_INVALID: assert(false);
            }
        }

        if(fieldType == OpcodeEncFieldType_LITERAL && value != field.value) {
            return OpcodeDecodeErr_NOT_COMPAT; // Literal is not present in code
        }

        fields[fieldType] = value;
    }

    if(reader->len != 0) {
        return OpcodeDecodeErr_INVALID; // Byte left not fully consumed
    }

    return OpcodeDecodeErr_OK;
}

static OpcodeType resolve_type(const OpcodeEncType encType) {
    // TODO: Verify this keeps being true after encoding access refactoring
    return (OpcodeType) encType;
}

int OpcodeEncoding_decode(const OpcodeEncoding *encoding, Opcode *opcode, const uint8_t code[], const uint8_t codeEnd[]) {
    bool hasField[OpcodeEncFieldType_COUNT] = {0};
    uint8_t fields[OpcodeEncFieldType_COUNT] = {0};

    #define FIELD(field) fields[OpcodeEncFieldType_##field]
    #define HAS_FIELD(field) hasField[OpcodeEncFieldType_##field]

    CodeReader codeReader = CodeReader_new(code, codeEnd);

    const OpcodeDecodeErr decodeErr = decode_fields(hasField, fields, encoding->fields, &codeReader);
    if(decodeErr) {
        return decodeErr;
    }
    if(opcode == NULL) {
        // Just checking if encoding is compatible
        return OpcodeDecodeErr_OK;
    }

    const uint8_t mod = FIELD(MOD);
    const uint8_t rm = FIELD(RM);
    const bool w = FIELD(W);
    const bool s = FIELD(S);
    const bool d = FIELD(D);

    const bool directAccess = mod == B8(00) && rm == B8(110);
    const RegSize dispLen = directAccess || mod == B8(10) ? RegSize_WORD : mod == B8(01) ? RegSize_BYTE : RegSize_NONE;
    const RegSize dataLen = HAS_FIELD(DATA_IF_W) && w && !s ? RegSize_WORD : HAS_FIELD(DATA) ? RegSize_BYTE : RegSize_NONE;
    const RegSize ipincLen = HAS_FIELD(IPINC16) ? RegSize_WORD : HAS_FIELD(IPINC8) ? RegSize_BYTE : RegSize_NONE;

    CodeReaderErr readErr;

    int16_t displacement;
    if((readErr = CodeReader_get_bytes_and_advance(&codeReader, dispLen, &displacement)) < 0) {
        return readErr;
    }

    int16_t data;
    if((readErr = CodeReader_get_bytes_and_advance(&codeReader, dataLen, &data)) < 0) {
        return readErr;
    }

    int16_t ipinc;
    if((readErr = CodeReader_get_bytes_and_advance(&codeReader, ipincLen, &ipinc)) < 0) {
        return readErr;
    }

    opcode->type = resolve_type(encoding->type);
    opcode->dst.type = OpcodeArgType_NONE;
    opcode->src.type = OpcodeArgType_NONE;

    OpcodeArg *regArg = d ? &opcode->dst : &opcode->src;
    OpcodeArg *rmArg = d ? &opcode->src : &opcode->dst;

    if(hasField[OpcodeEncFieldType_REG]) {
        regArg->type = OpcodeArgType_REGISTER;
        regArg->reg = resolve_reg_access(FIELD(REG), w);
    }

    if(hasField[OpcodeEncFieldType_MOD]) {
        if(mod == B8(11)) {
            // Register Mode
            rmArg->type = OpcodeArgType_REGISTER;
            rmArg->reg = resolve_reg_access(rm, w);
        } else {
            // Memory mode
            rmArg->type = OpcodeArgType_MEMORY;
            rmArg->mem = resolve_mem_access(rm, displacement, directAccess);
        }
    }

    // Immediate argument is the one not filled yet, or dstArg if only one arg
    OpcodeArg *immArg;
    if(regArg->type == OpcodeArgType_NONE && rmArg->type == OpcodeArgType_NONE) {
        immArg = &opcode->dst;
    } else if(regArg->type == OpcodeArgType_NONE) {
        immArg = regArg;
    } else if(rmArg->type == OpcodeArgType_NONE) {
        immArg = rmArg;
    } else {
        return (int) (codeReader.code - code); // No immediate arg -> We are done
    }

    if(dataLen) {
        immArg->type = OpcodeArgType_IMMEDIATE;
        immArg->imm.value = data;
        immArg->imm.size = s ? RegSize_WORD : dataLen;
    }

    if(ipincLen) {
        immArg->type = OpcodeArgType_IPINC;
        immArg->imm.value = ipinc;
        immArg->imm.size = ipincLen;
    }

    return (int) (codeReader.code - code);

    #undef HAS_FIELD
    #undef FIELD
}
