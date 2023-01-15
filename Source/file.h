#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "quakedef.h"

struct Q_File {
    uint8_t* ptr;
    size_t offset;
    size_t size;
};

typedef struct Q_File Q_File;

Q_File* Q_fopen(const char* filename, const char* mode);
void Q_fclose(Q_File* file);

inline size_t Q_fread(void* buffer, size_t size, size_t count, Q_File* stream) {
    size_t bytes_to_read = min(size * count, stream->size - stream->offset);
    if(bytes_to_read > 0) {
        memcpy(buffer, stream->ptr + stream->offset, bytes_to_read);
        stream->offset += bytes_to_read;
    }

    return bytes_to_read;
}

inline unsigned short Q_fgetLittleShort(Q_File* stream) {
    unsigned short out = 0;
    Q_fread(&out, 1, 2, stream);
    return out;
}

inline unsigned char Q_fgetc(Q_File* stream) {
    unsigned char c = (unsigned char)*(stream->ptr + stream->offset);
    ++stream->offset;
    return c;
}

inline void Q_fseek(Q_File* stream, size_t offset, size_t type) {
    if(type == SEEK_END) {
        stream->offset = stream->size;
    } else if(type == SEEK_SET) {
        stream->offset = offset;
    } else if(type == SEEK_CUR) {
        stream->offset += offset;
    }

    if(stream->offset > stream->size) {
        stream->offset = stream->size;
    }
}
