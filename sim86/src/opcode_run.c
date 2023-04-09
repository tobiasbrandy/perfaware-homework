#include "opcode_run.h"

#include <stdlib.h>

typedef void (*OpcodeF)(const Opcode *opcode, Memory *memory);

static void NONE(const Opcode *opcode, Memory *memory) {
    fprintf(stderr, "Invalid opcode type!");
    abort();
}

static void MOV(const Opcode *opcode, Memory *memory) {
    printf("mov!");
}

static void ADD(const Opcode *opcode, Memory *memory) {
    // TODO
}

static void ADC(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void SUB(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void SBB(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void CMP(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void AND(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void OR(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void XOR(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JE(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JL(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JLE(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JB(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JBE(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JP(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JO(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JS(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JNE(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JNL(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JNLE(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JNB(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JNBE(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JNP(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JNO(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JNS(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void LOOP(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void LOOPE(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void LOOPNE(const Opcode *opcode, Memory *memory) {
    // TODO
}
static void JCXZ(const Opcode *opcode, Memory *memory) {
    // TODO
}

void simulate_run(const Opcode *opcode, Memory *memory) {
    static OpcodeF ops[OpcodeType_COUNT] = {
            [OpcodeType_NONE] = NONE,

            #define OPCODE(name, ...) [OpcodeType_##name] = name,
            #define SUB_OP(...)
            #include "opcode_encoding_table.inl"
    };
    ops[opcode->type](opcode, memory);
}
