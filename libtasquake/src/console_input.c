#include "libtasquake/console_input.h"

#ifdef __GNUC__
#define STDIN 0
#include <stdlib.h>
#include <string.h>

void tasquake_stdin_read(struct stdin_input* result) {
    int rval = select(1, &result->fds, NULL, NULL, NULL);

    if(rval < 0) {
        return;
    }
        
    size_t bytes_left = result->buffer_size - result->input_index;

    if(bytes_left == 0) {
        result->buffer_size <<= 1;
        result->buffer = realloc(result->buffer, result->buffer_size);
        bytes_left = result->buffer_size - result->input_index;
    }

    if(!FD_ISSET(STDIN, &result->fds)) {
        return;
    }

    ssize_t bytes_read = read(STDIN, result->buffer + result->input_index, bytes_left);

    if(bytes_read <= 0) {
        return;
    }

    size_t old_input_index = result->input_index;
    result->input_index += bytes_read;

    size_t i=old_input_index;

    while(i < result->input_index) {
        if(result->buffer[i] != '\n') {
            ++i;
            continue;
        }

        result->buffer[i] = '\0';
        result->callback(result->buffer);

        size_t bytes_left = result->input_index - i - 1;

        if(bytes_left > 0) {
            memmove(result->buffer, result->buffer+i+1, bytes_left-1);
        }

        i = 0;
        result->input_index = bytes_left;
    }
}

void tasquake_stdin_init(struct stdin_input* result) {
    FD_ZERO(&result->fds);
    FD_SET(STDIN, &result->fds);
    result->buffer_size = 4096;
    result->buffer = malloc(result->buffer_size);
    result->input_index = 0;
}

void tasquake_stdin_free(struct stdin_input* result) {
    free(result->buffer);
}

#endif