#ifndef SIM86_OPCODE_ENCODING_TABLE_H
#define SIM86_OPCODE_ENCODING_TABLE_H

#include <stdint.h>

#include "opcode_encoding/opcode_encoding.h"

typedef struct {
    uint16_t size;
    const OpcodeEncoding *table;
} OpcodeEncodingTable;

OpcodeEncodingTable OpcodeEncodingTable_get(void);

const OpcodeEncoding *OpcodeEncoding_find(const uint8_t *codeStart, const uint8_t *codeEnd);

#endif //SIM86_OPCODE_ENCODING_TABLE_H
