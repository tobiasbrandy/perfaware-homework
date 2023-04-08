#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "memory.h"
#include "opcode_encoding.h"
#include "opcode_encoding_table.h"
#include "opcode_decompile.h"

static void print_opcode_decoding_error(const OpcodeDecodeErr err) {
    switch(err) {
        case OpcodeDecodeErr_OK: break; // No error
        case OpcodeDecodeErr_NOT_COMPAT: {
            fprintf(stderr, "sim86: error: Opcode encoding used was not compatible with the actual code\n");
        } break;
        case OpcodeDecodeErr_END: {
            fprintf(stderr, "sim86: error: Code ended in the middle of an opcode\n");
        } break;
        case OpcodeDecodeErr_INVALID: {
            fprintf(stderr, "sim86: error: Invalid opcode code for encoding\n");
        } break;
    }
}

static int decompile86(FILE *out, const uint8_t *codeStart, const uint8_t *codeEnd) {
    fprintf(out, "bits 16\n\n");

    for(const uint8_t *codePtr = codeStart; codePtr < codeEnd; ) {
        const OpcodeEncoding *encoding = opcode_encoding_find(codePtr, codeEnd);
        if(encoding == NULL) {
            fprintf(stderr, "sim86: error: Unknown opcode '%#04x'\n", *codePtr);
            return EXIT_FAILURE;
        }

        Opcode opcode;
        const int sizeOrErr = OpcodeEncoding_decode(encoding, &opcode, codePtr, codeEnd);
        if(sizeOrErr < 0) {
            print_opcode_decoding_error((OpcodeDecodeErr) sizeOrErr);
            return EXIT_FAILURE;
        }

        decompile_opcode(&opcode, out);
        codePtr += sizeOrErr;
    }

    return EXIT_SUCCESS;
}

int main(int argc, const char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "sim86: error: Missing source file path to disassemble\n");
        fprintf(stderr, "Usage: sim86 <src_file>\n");
        return EXIT_FAILURE;
    }

    const char *srcFile = argv[1];

    Memory memory = Memory_create();

    FILE *file = fopen(srcFile, "rb");
    if(file == NULL) {
        fprintf(stderr, "sim86: error: open '%s': %s\n", srcFile, strerror(errno));
        return EXIT_FAILURE;
    }

    const int codeLen = Memory_load_code(&memory, file);
    if(codeLen == -1) {
        fprintf(stderr, "sim86: error: failed to read '%s' source file\n", srcFile);
        return EXIT_FAILURE;
    }

    fclose(file);

    const int ret = decompile86(stdout, memory.ram, memory.ram + codeLen);

    return ret;
}
