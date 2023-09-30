#ifndef ROW_H
#define ROW_H

#include <stdint.h>

typedef struct {
    uint32_t id;
    char username[32];
    char email[256];
} Row;

void serialize_row(Row* source, char* destination);
void deserialize_row(char* source, Row* destination);
void print_row(Row* row);

#endif
