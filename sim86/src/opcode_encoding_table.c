#include "opcode_encoding_table.h"

static const OpcodeEncoding table[] = {
    #include "opcode_encoding_table.inl"
};
static const size_t tableSize = sizeof(table) / sizeof(*table);

OpcodeEncodingTable OpcodeEncodingTable_get(void) {
    OpcodeEncodingTable ret = {
            .size = sizeof(table) / sizeof(*table),
            .table = table,
    };
    return ret;
}

const OpcodeEncoding *OpcodeEncoding_find(const uint8_t *codeStart, const uint8_t *codeEnd) {
    for(size_t i = 0; i < tableSize; ++i) {
        int err = OpcodeEncoding_decode(&table[i], NULL, codeStart, codeEnd);
        if(err >= 0) {
            return &table[i];
        }
        if(err != OpcodeDecodeErr_NOT_COMPAT) {
            return NULL;
        }
    }

    return NULL;
}
