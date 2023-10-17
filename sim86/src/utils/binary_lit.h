// Replacement Macros for binary literals
#ifndef SIM86_BINARY_LIT_H
#define SIM86_BINARY_LIT_H

// Internal Macro
#define INTERNAL_B8__(x) \
     (((x&0x0000000FLU)?1:0) \
    +((x&0x000000F0LU)?2:0) \
    +((x&0x00000F00LU)?4:0) \
    +((x&0x0000F000LU)?8:0) \
    +((x&0x000F0000LU)?16:0) \
    +((x&0x00F00000LU)?32:0) \
    +((x&0x0F000000LU)?64:0) \
    +((x&0xF0000000LU)?128:0))

// User Macros
#define B8(d) ((unsigned char)INTERNAL_B8__(0x##d##LU))
#define B16(dmsb,dlsb) (((unsigned short)INTERNAL_B8__(dmsb)<<8) + INTERNAL_B8__(dlsb))
#define B32(dmsb,db2,db3,dlsb) \
    (((unsigned long)INTERNAL_B8__(dmsb)<<24) \
    + ((unsigned long)INTERNAL_B8__(db2)<<16) \
    + ((unsigned long)INTERNAL_B8__(db3)<<8) \
    + INTERNAL_B8__(dlsb))


#endif //SIM86_BINARY_LIT_H
