#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>

typedef struct {
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

void print_prompt(void);
InputBuffer* new_input_buffer(void);
void read_input(InputBuffer*);

#endif
