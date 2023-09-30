#ifndef INPUT_C
#define INPUT_C

#include "input.h"
#include <stdlib.h>

void print_prompt(void) { printf("db > "); }

InputBuffer* new_input_buffer(void) {
    InputBuffer* input_buffer = (InputBuffer*) malloc(sizeof(InputBuffer));

    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}

void read_input(InputBuffer* input_buffer) {
    ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

    if (bytes_read <= 0) {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    input_buffer->input_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = 0;
}

#endif
