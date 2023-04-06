#include "opcode_encoding.table.h"

static OpcodeEncoding table[] = {
#include "opcode_encoding_table.inl"
};

OpcodeEncodingTable OpcodeEncodingTable_create() {
    OpcodeEncodingTable ret = {
        .size = sizeof(table) / sizeof(*table),
        .table = table,
    };
    return ret;
}
