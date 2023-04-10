#ifndef SIM86_OPCODE_ENCODING_H
#define SIM86_OPCODE_ENCODING_H

#include <stdint.h>

#include "opcode.h"

#define MAX_ENC_FIELDS 16

typedef enum {
    OpcodeEncType_NONE = 0,

    #define OPCODE(name, ...) OpcodeEncType_##name,
    #define SUB_OP(...)
    #include "opcode_encoding_table.inl"

    OpcodeEncType_COUNT,
} OpcodeEncType;

typedef enum {
    OpcodeEncFieldType_END = 0, // No more fields

    OpcodeEncFieldType_LITERAL,

    OpcodeEncFieldType_S,
    OpcodeEncFieldType_W,
    OpcodeEncFieldType_D,

    OpcodeEncFieldType_MOD,
    OpcodeEncFieldType_REG,
    OpcodeEncFieldType_RM,
    OpcodeEncFieldType_SR,

    OpcodeEncFieldType_DISP,
    OpcodeEncFieldType_DATA,
    OpcodeEncFieldType_IPINC8,
    OpcodeEncFieldType_IPINC16,

    OpcodeEncFieldType_DATA_IF_W,

    OpcodeEncFieldType_COUNT,
} OpcodeEncFieldType;

typedef struct {
    OpcodeEncFieldType type;
    uint8_t length;
    uint8_t value;
} OpcodeEncField;

typedef struct {
    OpcodeEncType type;
    OpcodeEncField fields[MAX_ENC_FIELDS];
} OpcodeEncoding;

typedef enum {
    OpcodeDecodeErr_OK          =  0,
    OpcodeDecodeErr_NOT_COMPAT  = -1, // Encoding and code are not compatible
    OpcodeDecodeErr_END         = -2, // Decoder reached end before decoding finished
    OpcodeDecodeErr_INVALID     = -3, // OpcodeEncoding is invalid
} OpcodeDecodeErr;

int OpcodeEncoding_decode(const OpcodeEncoding *encoding, Opcode *opcode, const uint8_t code[], const uint8_t codeEnd[]);

#endif //SIM86_OPCODE_ENCODING_H
