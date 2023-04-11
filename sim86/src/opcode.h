#ifndef SIM86_OPCODE_DECODE_H
#define SIM86_OPCODE_DECODE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    Register_AX = 0,
    Register_BX,
    Register_CX,
    Register_DX,
    Register_SP,
    Register_BP,
    Register_SI,
    Register_DI,
    Register_ES,
    Register_CS,
    Register_SS,
    Register_DS,
    Register_IP,
//    Register_flags,
    Register_COUNT,
} Register;

typedef enum {
    RegSize_BYTE = 1,
    RegSize_WORD = 2,
} RegSize;

typedef enum {
    RegOffset_NONE = 0,
    RegOffset_LOW,
    RegOffset_HIGH,
} RegOffset;

typedef enum {
    OpcodeType_NONE = 0,

    #define OPCODE(name, ...) OpcodeType_##name,
    #define SUB_OP(...)
    #include "opcode_encoding_table.inl"

    OpcodeType_COUNT,
} OpcodeType;

typedef struct {
    Register reg;
    RegSize size;
    RegOffset offset;
} OpcodeRegAccess;

typedef struct {
    OpcodeRegAccess reg;
    bool present;
} OpcodeAddrRegTerm;

typedef struct {
    OpcodeAddrRegTerm terms[2];
    int16_t displacement;
    RegSize size;
} OpcodeMemAccess;

typedef struct {
    int16_t value;
    RegSize size;
} OpcodeImmAccess;

typedef enum {
    OpcodeArgType_NONE = 0,
    OpcodeArgType_REGISTER,
    OpcodeArgType_MEMORY,
    OpcodeArgType_IMMEDIATE,
    OpcodeArgType_IPINC,
} OpcodeArgType;

typedef struct {
    OpcodeArgType type;
    union {
        OpcodeRegAccess reg;
        OpcodeMemAccess mem;
        OpcodeImmAccess imm;
        OpcodeImmAccess ipinc;
    };
} OpcodeArg;

typedef struct {
    OpcodeType type;
    OpcodeArg dst, src;
    uint8_t len;
} Opcode;

RegSize OpcodeArg_size(const OpcodeArg *arg);

int RegSize_max(RegSize size);

#endif //SIM86_OPCODE_DECODE_H
