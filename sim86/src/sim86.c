#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "memory.h"
#include "opcode_encoding.h"
#include "opcode_encoding_table.h"
#include "opcode_decompile.h"
#include "opcode_run.h"

static void print_usage(void) {
    fprintf(stderr, "Usage: sim86 <cmd> <src_file>\n");
    fprintf(stderr, "Available commands: decompile, run, trace\n");
}

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

static bool for_each_opcode(Opcode *opcode, const uint8_t **codePtr, const uint8_t *codeEnd) {
    if(*codePtr == codeEnd) {
        return false;
    }

    const OpcodeEncoding *encoding = opcode_encoding_find(*codePtr, codeEnd);
    if(encoding == NULL) {
        fprintf(stderr, "sim86: error: Unknown opcode '0x%02x'\n", **codePtr);
        exit(EXIT_FAILURE);
    }

    const int sizeOrErr = OpcodeEncoding_decode(encoding, opcode, *codePtr, codeEnd);
    if(sizeOrErr < 0) {
        print_opcode_decoding_error((OpcodeDecodeErr) sizeOrErr);
        exit(EXIT_FAILURE);
    }

    *codePtr += sizeOrErr;
    return true;
}

static void decompile86(FILE *out, const uint8_t *codeStart, const uint8_t *codeEnd) {
    fprintf(out, "bits 16\n\n");

    Opcode opcode;
    for(const uint8_t **codePtr = &codeStart; for_each_opcode(&opcode, codePtr, codeEnd); ) {
        decompile_opcode_to_file(&opcode, out);
        fputc('\n', out);
    }
}

static void run86(Memory *memory, const uint8_t *codeEnd, FILE *trace) {
    const uint8_t *codeStart = memory->ram;

    Opcode opcode;
    for(const uint8_t **codePtr = &codeStart; for_each_opcode(&opcode, codePtr, codeEnd); ) {
        if(trace) {
            decompile_opcode_to_file(&opcode, trace);
            fputs(" ; ", trace);
        }
        simulate_run(&opcode, memory, trace);
        if(trace) {
            fputs(" \n", trace);
        }
    }

    if(trace) {
        const uint16_t *regs = memory->registers;
        fprintf(trace, "\n"
            "Final registers:\n"
            "      ax: 0x%04x (%d)\n"
            "      bx: 0x%04x (%d)\n"
            "      cx: 0x%04x (%d)\n"
            "      dx: 0x%04x (%d)\n"
            "      sp: 0x%04x (%d)\n"
            "      bp: 0x%04x (%d)\n"
            "      si: 0x%04x (%d)\n"
            "      di: 0x%04x (%d)\n",
            regs[Register_AX], regs[Register_AX],
            regs[Register_BX], regs[Register_BX],
            regs[Register_CX], regs[Register_CX],
            regs[Register_DX], regs[Register_DX],
            regs[Register_SP], regs[Register_SP],
            regs[Register_BP], regs[Register_BP],
            regs[Register_SI], regs[Register_SI],
            regs[Register_DI], regs[Register_DI]
        );
    }
}

int main(int argc, const char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "sim86: error: Missing command and source file path\n");
        print_usage();
        return EXIT_FAILURE;
    }
    if(argc < 3) {
        fprintf(stderr, "sim86: error: Missing source file path\n");
        print_usage();
        return EXIT_FAILURE;
    }

    const char *cmd = argv[1];
    const char *srcFile = argv[2];

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

    const uint8_t *codeEnd = memory.ram + codeLen;

    if(!strcmp(cmd, "decompile"))   decompile86(stdout, memory.ram, codeEnd);
    else if(!strcmp(cmd, "run"))    run86(&memory, codeEnd, NULL);
    else if(!strcmp(cmd, "trace"))  run86(&memory, codeEnd, stdout);
    else {
        fprintf(stderr, "sim86: error: unknown command '%s'\n", cmd);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
