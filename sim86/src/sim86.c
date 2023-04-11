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

static bool parse_opcode(Opcode *opcode, Memory *mem) {
    if(Memory_code_ended(mem)) {
        return false;
    }

    const uint8_t *codePtr = Memory_code_ptr(mem);
    const uint8_t *codeEnd = mem->codeEnd;

    const OpcodeEncoding *encoding = OpcodeEncoding_find(codePtr, codeEnd);
    if(encoding == NULL) {
        fprintf(stderr, "sim86: error: Unknown opcode '0x%02x'\n", *codePtr);
        exit(EXIT_FAILURE);
    }

    const OpcodeDecodeErr err = OpcodeEncoding_decode(encoding, opcode, codePtr, codeEnd);
    if(err) {
        print_opcode_decoding_error(err);
        exit(EXIT_FAILURE);
    }

    return true;
}

static void decompile86(Memory *memory, FILE *out) {
    fprintf(out, "bits 16\n\n");

    for(Opcode opcode; parse_opcode(&opcode, memory); memory->registers[Register_IP] += opcode.len) {
        Opcode_decompile_to_file(&opcode, out);
        fputc('\n', out);
    }
}

static void run86(Memory *memory, FILE *trace) {
    for(Opcode opcode; parse_opcode(&opcode, memory); ) {
        if(trace) {
            Opcode_decompile_to_file(&opcode, trace);
            fputs(" ;", trace);
        }

        Opcode_run(&opcode, memory, trace);

        if(trace) {
            fputc('\n', trace);
        }
    }

    if(trace) {
        // Full register trace
        const uint16_t *regs = memory->registers;
        OpcodeRegAccess regAccess = {.reg=0, .size=RegSize_WORD, .offset=RegOffset_NONE};

        fprintf(trace, "\nFinal registers:\n");
        for(Register reg = 0; reg < Register_COUNT; ++reg) {
            const uint16_t val = regs[reg];
            if(val) {
                regAccess.reg = reg;
                fprintf(trace, "      %s: 0x%04x (%d)\n", OpcodeRegAccess_decompile(&regAccess), val, val);
            }
        }

        char flagsBuf[FLAG_COUNT + 1];
        Flags_serialize(&memory->flags, flagsBuf);
        if(*flagsBuf) {
            fprintf(trace, "   flags: %s\n", flagsBuf);
        }
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

    if(!Memory_load_code(&memory, file)) {
        fprintf(stderr, "sim86: error: failed to read '%s' source file\n", srcFile);
        return EXIT_FAILURE;
    }

    fclose(file);

    if(!strcmp(cmd, "decompile"))   decompile86(&memory, stdout);
    else if(!strcmp(cmd, "run"))    run86(&memory, NULL);
    else if(!strcmp(cmd, "trace"))  run86(&memory, stdout);
    else {
        fprintf(stderr, "sim86: error: unknown command '%s'\n", cmd);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
