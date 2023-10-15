#ifndef NODE_H
#define NODE_H

#include "row.h"
#include "table.h"
#include "cursor.h"
#include <stdint.h>

typedef enum { 
    NODE_INTERNAL, 
    NODE_LEAF 
} NodeType;

NodeType get_node_type(uint8_t* node);

void set_node_type(void* node, NodeType type);

uint32_t* leaf_node_num_cells(uint8_t* node);

uint8_t* leaf_node_cell(uint8_t* node, uint32_t cell_num);

uint32_t* leaf_node_key(uint8_t* node, uint32_t cell_num);

uint8_t* leaf_node_value(uint8_t* node, uint32_t cell_num);

void initialize_leaf_node(uint8_t* node);

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value);

void print_leaf_node(uint8_t* node);

#endif
