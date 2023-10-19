#ifndef CURSOR_H
#define CURSOR_H

#include "table.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    Table* table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table; // Indicates a position past the last element
} Cursor;

uint8_t* cursor_value(Cursor* cursor);
void cursor_advance(Cursor* cursor);
Cursor* table_start(Table* table);
Cursor* table_find(Table* table, uint32_t key);
Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key);
Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key);

#endif
