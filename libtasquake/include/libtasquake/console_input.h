#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>

#ifdef __GNUC__
#include <stdio.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

typedef void (*tasquake_line_callback)(const char*);

struct stdin_input {
    fd_set fds;
    size_t input_index;
    size_t buffer_size;
    char* buffer;
    tasquake_line_callback callback;
};

void tasquake_stdin_read(struct stdin_input* result);
void tasquake_stdin_init(struct stdin_input* result);
void tasquake_stdin_free(struct stdin_input* result);

#endif

#ifdef __cplusplus
}
#endif
