#ifndef PERFAWARE_BIN_FILE_READER_H
#define PERFAWARE_BIN_FILE_READER_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    FILE *file;
    uint16_t size, pos;
    uint8_t *buf;
} BinFileReader;

typedef enum {
    BinFileReader_Status_OK = 0,
    BinFileReader_Status_EOF,
    BinFileReader_Status_ERROR,
} BinFileReader_Status;

BinFileReader BinFileReader_create(FILE *file, uint8_t buf[], uint16_t size);

BinFileReader_Status BinFileReader_status(const BinFileReader *reader);

uint8_t *BinFileReader_read_bytes(BinFileReader *reader, uint16_t n);

#endif //PERFAWARE_BIN_FILE_READER_H
