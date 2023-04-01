#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "bin_file_reader.h"
#include "opcodes.h"

#define BUF_SIZE 64
static uint8_t buf[BUF_SIZE];

void print_opcode_error(Opcode_Err err) {
    switch(err) {
        case Opcode_Err_OK: {
            // No error
            break;
        }
        case Opcode_Err_EOF: {
            fprintf(stderr, "sim86: error: Source file ended mid command\n");
            break;
        }
        case Opcode_Err_FILE_ERR: {
            fprintf(stderr, "sim86: error: Failure while reading source file mid command\n");
            break;
        }
        case Opcode_Err_INVALID_ARG: {
            fprintf(stderr, "sim86: error: Invalid command argument in source file\n");
            break;
        }
        case Opcode_Err_UNREACHABLE: {
            fprintf(stderr, "sim86: panic: Unreachable code executed\n");
            abort();
        }
    }
}

int disassemble86(FILE *out, BinFileReader *reader) {
    fprintf(out, "bits 16\n\n");

    uint8_t *opcode;
    while((opcode = BinFileReader_read_bytes(reader, 1)) != NULL) {
        Opcode_Err opcodeErr = opcodes[*opcode](out, reader, *opcode);
        if(opcodeErr) {
            print_opcode_error(opcodeErr);
            return EXIT_FAILURE;
        }
    }

    if(BinFileReader_status(reader) == BinFileReader_Status_ERROR) {
        fprintf(stderr, "sim86: error: Failure while reading source file\n");
        return EXIT_FAILURE;
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

    FILE *file = fopen(srcFile, "rb");
    if(file == NULL) {
        fprintf(stderr, "sim86: error: open '%s': %s\n", srcFile, strerror(errno));
        return EXIT_FAILURE;
    }

    FILE *out = stdout;
    BinFileReader reader = BinFileReader_create(file, buf, BUF_SIZE);

    int ret = disassemble86(out, &reader);

    fclose(file);

    return ret;
}
