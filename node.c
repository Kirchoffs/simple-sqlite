#include "node.h"
#include "constants.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

NodeType get_node_type(uint8_t* node) {
    uint8_t value = *((uint8_t*)(node + NODE_TYPE_OFFSET));
    return (NodeType)value;
}

void set_node_type(uint8_t* node, NodeType type) {
    uint8_t value = type;
    *((uint8_t*)(node + NODE_TYPE_OFFSET)) = value;
}

bool is_node_root(uint8_t* node) {
    uint8_t value = *((uint8_t*)(node + IS_ROOT_OFFSET));
    return (bool)value;
}

void set_node_root(uint8_t* node, bool is_root) {
    uint8_t value = is_root;
    *((uint8_t*)(node + IS_ROOT_OFFSET)) = value;
}

uint32_t* node_parent(uint8_t* node) { 
    return (uint32_t*)(node + PARENT_POINTER_OFFSET); 
}

uint32_t* leaf_node_num_cells(uint8_t* node) {
    return (uint32_t*)(node + LEAF_NODE_NUM_CELLS_OFFSET);
}

uint32_t* leaf_node_next_leaf(void* node) {
    return node + LEAF_NODE_NEXT_LEAF_OFFSET;
}

uint8_t* leaf_node_cell(uint8_t* node, uint32_t cell_num) {
    return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(uint8_t* node, uint32_t cell_num) {
    return (uint32_t*)leaf_node_cell(node, cell_num);
}

uint8_t* leaf_node_value(uint8_t* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

void initialize_leaf_node(uint8_t* node) {
    set_node_type(node, NODE_LEAF);
    set_node_root(node, false);
    *leaf_node_num_cells(node) = 0;
    *leaf_node_next_leaf(node) = 0;
}

uint32_t get_unused_page_num(Pager* pager) { 
    return pager->num_pages; 
}

ExecuteResult leaf_node_insert(Cursor* cursor, uint32_t key, Row* value) {
    uint8_t* node = get_page(cursor->table->pager, cursor->page_num);

    uint32_t num_cells = *leaf_node_num_cells(node);
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        return leaf_node_split_and_insert(cursor, key, value);
    }

    if (cursor->cell_num < num_cells) {
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->cell_num)) = key;
    serialize_row(value, (char*)leaf_node_value(node, cursor->cell_num));

    return EXECUTE_SUCCESS;
}

ExecuteResult leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value) {
    uint8_t* old_node = get_page(cursor->table->pager, cursor->page_num);
    uint32_t old_node_max_key = get_node_max_key(old_node);
    uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
    uint8_t* new_node = get_page(cursor->table->pager, new_page_num);
    initialize_leaf_node(new_node);
    *node_parent(new_node) = *node_parent(old_node);
    *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
    *leaf_node_next_leaf(old_node) = new_page_num;

    // Process totally LEAF_NODE_MAX_CELLS + 1 cells
    for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
        uint8_t* destination_node;
        if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
            destination_node = new_node;
        } else {
            destination_node = old_node;
        }

        uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
        uint8_t* destination = leaf_node_cell(destination_node, index_within_node);

        if (i == cursor->cell_num) {
            serialize_row(value, (char*)leaf_node_value(destination_node, index_within_node));
            *leaf_node_key(destination_node, index_within_node) = key;
        } else if (i > cursor->cell_num) {
            memcpy(destination, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
        } else {
            memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
    *(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

    if (is_node_root(old_node)) {
        create_new_root(cursor->table, new_page_num);
        return EXECUTE_SUCCESS;
    } else {
        uint32_t parent_page_num = *node_parent(old_node);
        uint8_t* parent_node = get_page(cursor->table->pager, parent_page_num);
        uint32_t old_node_max_key_new = get_node_max_key(old_node);
        update_internal_node_key(parent_node, old_node_max_key, old_node_max_key_new);
        internal_node_insert(cursor->table, parent_page_num, new_page_num);
        return EXECUTE_SUCCESS;
    }
}

void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    uint8_t* parent = get_page(table->pager, parent_page_num);
    uint8_t* child = get_page(table->pager, child_page_num);
    uint32_t child_max_key = get_node_max_key(child);
    uint32_t index = internal_node_find_child(parent, child_max_key);

    uint32_t parent_original_num_keys = *internal_node_num_keys(parent);
    *internal_node_num_keys(parent) = parent_original_num_keys + 1;

    if (parent_original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
        printf("Need to implement splitting internal node\n");
        exit(EXIT_FAILURE);
    }

    uint32_t right_child_page_num = *internal_node_right_child(parent);
    uint8_t* right_child = get_page(table->pager, right_child_page_num);

    if (child_max_key > get_node_max_key(right_child)) {
        *internal_node_child(parent, parent_original_num_keys) = right_child_page_num;
        *internal_node_key(parent, parent_original_num_keys) = get_node_max_key(right_child);
        *internal_node_right_child(parent) = child_page_num;
    } else {
        for (uint32_t i = parent_original_num_keys; i > index; i--) {
            uint32_t* destination = internal_node_cell(parent, i);
            uint32_t* source = internal_node_cell(parent, i - 1);
            memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);
        }
        *internal_node_child(parent, index) = child_page_num;
        *internal_node_key(parent, index) = child_max_key;
    }
}

uint32_t* internal_node_num_keys(uint8_t* node) {
    return (uint32_t*)(node + INTERNAL_NODE_NUM_KEYS_OFFSET);
}

uint32_t* internal_node_right_child(uint8_t* node) {
    return (uint32_t*)(node + INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

uint32_t* internal_node_cell(uint8_t* node, uint32_t cell_num) {
    return (uint32_t*)(node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE);
}

uint32_t* internal_node_child(uint8_t* node, uint32_t child_num) {
    uint32_t num_keys = *internal_node_num_keys(node);
    if (child_num > num_keys) {
        printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
        exit(EXIT_FAILURE);
    } else if (child_num == num_keys) {
        return internal_node_right_child(node);
    } else {
        return internal_node_cell(node, child_num);
    }
}

uint32_t* internal_node_key(uint8_t* node, uint32_t key_num) {
    return (uint32_t*)((uint8_t*)internal_node_cell(node, key_num) + INTERNAL_NODE_KEY_OFFSET);
}

void initialize_internal_node(uint8_t* node) {
    set_node_type(node, NODE_INTERNAL);
    set_node_root(node, false);
    *internal_node_num_keys(node) = 0;
}

void update_internal_node_key(uint8_t* node, uint32_t old_key, uint32_t new_key) {
    uint32_t old_child_index = internal_node_find_child(node, old_key);
    *internal_node_key(node, old_child_index) = new_key;
}

/**
 *
 * Return the index of the child which should contain the given key. 
 * 
 * For each key, there is a child left to it which has keys less than or equal to this key,
 * 
 */
uint32_t internal_node_find_child(uint8_t* node, uint32_t key) {
    uint32_t num_keys = *internal_node_num_keys(node);

    uint32_t l_index = 0;
    uint32_t r_index = num_keys;

    while (l_index < r_index) {
        uint32_t index = l_index + (r_index - l_index) / 2;
        uint32_t key_to_right = *internal_node_key(node, index);
        if (key_to_right < key) {
            l_index = index + 1;
        } else {
            r_index = index;
        }
    }

    return l_index;
}

uint32_t get_node_max_key(uint8_t* node) {
    switch (get_node_type(node)) {
        case NODE_INTERNAL:
            return *internal_node_key(node, *internal_node_num_keys(node) - 1);
        case NODE_LEAF:
            return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
    }
}

void create_new_root(Table* table, uint32_t right_child_page_num) {
    uint8_t* root = get_page(table->pager, table->root_page_num);
    uint8_t* right_child = get_page(table->pager, right_child_page_num);

    uint32_t left_child_page_num = get_unused_page_num(table->pager);
    uint8_t* left_child = get_page(table->pager, left_child_page_num);

    memcpy(left_child, root, PAGE_SIZE);
    set_node_root(left_child, false);

    initialize_internal_node(root);
    set_node_root(root, true);
    *internal_node_num_keys(root) = 1;
    *internal_node_child(root, 0) = left_child_page_num;
    uint32_t left_child_max_key = get_node_max_key(left_child);
    *internal_node_key(root, 0) = left_child_max_key;
    *internal_node_right_child(root) = right_child_page_num;
    *node_parent(left_child) = table->root_page_num;
    *node_parent(right_child) = table->root_page_num;
}
