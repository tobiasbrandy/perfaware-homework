#include "binary_lit.h"

#ifndef OPCODE
#define OPCODE(name, ...) {OpcodeEncType_##name, {__VA_ARGS__, {OpcodeEncFieldType_END}}},
#endif

#ifndef SUB_OP
#define SUB_OP OPCODE
#endif

#define B(bits) {OpcodeEncFieldType_LITERAL, sizeof(#bits)-1, B8(bits)}
#define D {OpcodeEncFieldType_D, 1, 0}
#define S {OpcodeEncFieldType_S, 1, 0}
#define W {OpcodeEncFieldType_W, 1, 0}
//#define V {OpcodeEncFieldType_V, 1, 0}
//#define Z {OpcodeEncFieldType_Z, 1, 0}

#define RM {OpcodeEncFieldType_RM, 3, 0}
#define MOD {OpcodeEncFieldType_MOD, 2, 0}
#define REG {OpcodeEncFieldType_REG, 3, 0}

#define DISP {OpcodeEncFieldType_DISP, 0, 0}
#define DATA {OpcodeEncFieldType_DATA, 0, 0}
#define IPINC8 {OpcodeEncFieldType_IPINC8, 0, 0}
#define DATA_IF_W {OpcodeEncFieldType_DATA_IF_W, 0, 0}

#define SET_D(value) {OpcodeEncFieldType_D, 0, value}
#define SET_S(value) {OpcodeEncFieldType_S, 0, value}
#define SET_W(value) {OpcodeEncFieldType_W, 0, value}

#define SET_RM(value) {OpcodeEncFieldType_RM, 0, value}
#define SET_MOD(value) {OpcodeEncFieldType_MOD, 0, value}
#define SET_REG(value) {OpcodeEncFieldType_REG, 0, value}

#define FROM_REG SET_D(0)
#define TO_REG SET_D(1)
#define ACC SET_REG(0)
#define DIRECT_ACCESS SET_MOD(0), SET_RM(B8(110))

OPCODE(MOV, B(100010), D, W, MOD, REG, RM)
SUB_OP(MOV, B(1100011), W, MOD, B(000), RM, DATA, DATA_IF_W, FROM_REG)
SUB_OP(MOV, B(1011), W, REG, DATA, DATA_IF_W, TO_REG)
SUB_OP(MOV, B(1010000), W, DISP, DIRECT_ACCESS, TO_REG, ACC)
SUB_OP(MOV, B(1010001), W, DISP, DIRECT_ACCESS, FROM_REG, ACC)

OPCODE(ADD, B(000000), D, W, MOD, REG, RM)
SUB_OP(ADD, B(100000), S, W, MOD, B(000), RM, DATA, DATA_IF_W)
SUB_OP(ADD, B(0000010), W, DATA, DATA_IF_W, TO_REG, ACC)

OPCODE(ADC, B(000100), D, W, MOD, REG, RM)
SUB_OP(ADC, B(100000), S, W, MOD, B(010), RM, DATA, DATA_IF_W)
SUB_OP(ADC, B(0001010), W, DATA, DATA_IF_W, TO_REG, ACC)

OPCODE(SUB, B(001010), D, W, MOD, REG, RM)
SUB_OP(SUB, B(100000), S, W, MOD, B(101), RM, DATA, DATA_IF_W)
SUB_OP(SUB, B(0010110), W, DATA, DATA_IF_W, TO_REG, ACC)

OPCODE(SBB, B(000110), D, W, MOD, REG, RM)
SUB_OP(SBB, B(100000), S, W, MOD, B(011), RM, DATA, DATA_IF_W)
SUB_OP(SBB, B(0001110), W, DATA, DATA_IF_W, TO_REG, ACC)

OPCODE(CMP, B(001110), D, W, MOD, REG, RM)
SUB_OP(CMP, B(100000), S, W, MOD, B(111), RM, DATA, DATA_IF_W)
SUB_OP(CMP, B(0011110), W, DATA, DATA_IF_W, TO_REG, ACC)

OPCODE(AND, B(001000), D, W, MOD, REG, RM)
SUB_OP(AND, B(100000), S, W, MOD, B(100), RM, DATA, DATA_IF_W)
SUB_OP(AND, B(0010010), W, DATA, DATA_IF_W, TO_REG, ACC)

OPCODE(OR, B(000010), D, W, MOD, REG, RM)
SUB_OP(OR, B(100000), S, W, MOD, B(001), RM, DATA, DATA_IF_W)
SUB_OP(OR, B(0000110), W, DATA, DATA_IF_W, TO_REG, ACC)

OPCODE(XOR, B(001100), D, W, MOD, REG, RM)
SUB_OP(XOR, B(100000), S, W, MOD, B(110), RM, DATA, DATA_IF_W)
SUB_OP(XOR, B(0011010), W, DATA, DATA_IF_W, TO_REG, ACC)

OPCODE(JE,      B(01110100), IPINC8)
OPCODE(JL,      B(01111100), IPINC8)
OPCODE(JLE,     B(01111110), IPINC8)
OPCODE(JB,      B(01110010), IPINC8)
OPCODE(JBE,     B(01110110), IPINC8)
OPCODE(JP,      B(01111010), IPINC8)
OPCODE(JO,      B(01110000), IPINC8)
OPCODE(JS,      B(01111000), IPINC8)
OPCODE(JNE,     B(01110101), IPINC8)
OPCODE(JNL,     B(01111101), IPINC8)
OPCODE(JNLE,    B(01111111), IPINC8)
OPCODE(JNB,     B(01110011), IPINC8)
OPCODE(JNBE,    B(01110111), IPINC8)
OPCODE(JNP,     B(01111011), IPINC8)
OPCODE(JNO,     B(01110001), IPINC8)
OPCODE(JNS,     B(01111001), IPINC8)
OPCODE(LOOP,    B(11100010), IPINC8)
OPCODE(LOOPE,   B(11100001), IPINC8)
OPCODE(LOOPNE,  B(11100000), IPINC8)
OPCODE(JCXZ,    B(11100011), IPINC8)

#undef OPCODE
#undef SUB_OP

#undef B
#undef D
#undef S
#undef W
//#undef V
//#undef Z

#undef RM
#undef MOD
#undef REG

#undef DISP
#undef DATA
#undef IPINC8
#undef DATA_IF_W
#undef DATA_IF_SW

#undef SET_D
#undef SET_S
#undef SET_W

#undef SET_RM
#undef SET_MOD
#undef SET_REG

#undef FROM_REG
#undef TO_REG
#undef ACC
#undef DIRECT_ACCESS
