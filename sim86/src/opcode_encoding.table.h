#ifndef PERFAWARE_OPCODE_ENCODING_TABLE_H
#define PERFAWARE_OPCODE_ENCODING_TABLE_H

#include <stdint.h>

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

    OpcodeEncFieldType_DISP,
    OpcodeEncFieldType_DATA,
    OpcodeEncFieldType_IPINC8,

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

typedef struct {
    uint16_t size;
    const OpcodeEncoding *table;
} OpcodeEncodingTable;

OpcodeEncodingTable OpcodeEncodingTable_create();

#endif //PERFAWARE_OPCODE_ENCODING_TABLE_H
