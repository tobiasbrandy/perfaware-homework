#include "bin_file_reader.h"

#include <stdbool.h>
#include <string.h>

static bool hydrate(BinFileReader *reader);

BinFileReader BinFileReader_create(FILE *file, uint8_t buf[], uint16_t size) {
    const BinFileReader ret = {
        .file = file,
        .buf = buf,
        .size = size,
        .pos = size,
    };
    return ret;
}

BinFileReader_Status BinFileReader_status(const BinFileReader *reader) {
    if(reader->file == NULL) {
        if(reader->buf == NULL) {
            return BinFileReader_Status_ERROR;
        }

        return BinFileReader_Status_EOF;
    }

    return BinFileReader_Status_OK;
}

uint8_t *BinFileReader_read_bytes(BinFileReader *reader, uint16_t n) {
    if(reader->pos + n > reader->size) {
        if(reader->file == NULL) {
            return NULL;
        }

        if(!hydrate(reader) || reader->pos + n > reader->size) {
            return NULL;
        }
    }

    uint8_t *ret = reader->buf + reader->pos;
    reader->pos += n;
    return ret;
}

static bool hydrate(BinFileReader *reader) {
    uint16_t remaining = reader->size - reader->pos;
    memcpy(reader->buf, reader->buf + reader->pos, remaining);
    reader->pos = 0;

    uint16_t n = (uint16_t) fread(reader->buf + remaining, 1, reader->size, reader->file);

    if(n < reader->size) {
        FILE *file = reader->file;
        reader->file = NULL; // File not usable

        if(ferror(file)) {
            // Read error
            reader->buf = NULL; // Buf not usable
            return false;
        }
    }

    reader->size = n;
    return true;
}
