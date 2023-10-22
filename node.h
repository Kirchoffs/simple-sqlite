#ifndef NODE_H
#define NODE_H

#include "row.h"
#include "table.h"
#include "cursor.h"
#include "statement.h"
#include <stdint.h>

typedef enum { 
    NODE_INTERNAL, 
    NODE_LEAF 
} NodeType;

NodeType get_node_type(uint8_t* node);

void set_node_type(uint8_t* node, NodeType type);

bool is_node_root(uint8_t* node);

void set_node_root(uint8_t* node, bool is_root);

uint32_t* node_parent(uint8_t* node);

uint32_t* leaf_node_num_cells(uint8_t* node);

uint32_t* leaf_node_next_leaf(void* node);

uint8_t* leaf_node_cell(uint8_t* node, uint32_t cell_num);

uint32_t* leaf_node_key(uint8_t* node, uint32_t cell_num);

uint8_t* leaf_node_value(uint8_t* node, uint32_t cell_num);

void initialize_leaf_node(uint8_t* node);

uint32_t get_unused_page_num(Pager* pager);

ExecuteResult leaf_node_insert(Cursor* cursor, uint32_t key, Row* value);

ExecuteResult leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value);

void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num);

uint32_t* internal_node_num_keys(uint8_t* node);

uint32_t* internal_node_right_child(uint8_t* node);

uint32_t* internal_node_cell(uint8_t* node, uint32_t cell_num);

uint32_t* internal_node_child(uint8_t* node, uint32_t child_num);

uint32_t* internal_node_key(uint8_t* node, uint32_t key_num);

void initialize_internal_node(uint8_t* node);

void update_internal_node_key(uint8_t* node, uint32_t old_key, uint32_t new_key);

uint32_t internal_node_find_child(uint8_t* node, uint32_t key);

uint32_t get_node_max_key(uint8_t* node);

void create_new_root(Table* table, uint32_t right_child_page_num);

#endif
