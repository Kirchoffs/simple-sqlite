#include "node.h"
#include "constants.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INVALID_PAGE_NUM UINT32_MAX

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

uint32_t* node_parent_page_num(uint8_t* node) { 
    return (uint32_t*)(node + PARENT_POINTER_OFFSET); 
}

uint32_t* leaf_node_num_cells(uint8_t* node) {
    return (uint32_t*)(node + LEAF_NODE_NUM_CELLS_OFFSET);
}

uint32_t* leaf_node_next_leaf_page_num(uint8_t* node) {
    return (uint32_t*)(node + LEAF_NODE_NEXT_LEAF_OFFSET);
}

uint32_t* leaf_node_cell(uint8_t* node, uint32_t cell_num) {
    return (uint32_t*)(node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE);
}

uint32_t* leaf_node_key(uint8_t* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num);
}

uint8_t* leaf_node_value(uint8_t* node, uint32_t cell_num) {
    return (uint8_t*)leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

void initialize_leaf_node(uint8_t* node) {
    set_node_type(node, NODE_LEAF);
    set_node_root(node, false);
    *leaf_node_num_cells(node) = 0;
    *leaf_node_next_leaf_page_num(node) = 0;
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
    uint32_t old_node_max_key = get_node_max_key(cursor->table->pager, old_node);
    uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
    uint8_t* new_node = get_page(cursor->table->pager, new_page_num);
    initialize_leaf_node(new_node);
    *node_parent_page_num(new_node) = *node_parent_page_num(old_node);
    *leaf_node_next_leaf_page_num(new_node) = *leaf_node_next_leaf_page_num(old_node);
    *leaf_node_next_leaf_page_num(old_node) = new_page_num;

    // Process totally LEAF_NODE_MAX_CELLS + 1 cells
    for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
        uint8_t* destination_node;
        if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
            destination_node = new_node;
        } else {
            destination_node = old_node;
        }

        uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
        uint32_t* destination = leaf_node_cell(destination_node, index_within_node);

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
        uint32_t parent_page_num = *node_parent_page_num(old_node);
        uint8_t* parent_node = get_page(cursor->table->pager, parent_page_num);
        uint32_t old_node_max_key_new = get_node_max_key(cursor->table->pager, old_node);
        update_internal_node_key(parent_node, old_node_max_key, old_node_max_key_new);
        internal_node_insert(cursor->table, parent_page_num, new_page_num);
        return EXECUTE_SUCCESS;
    }
}

void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    uint8_t* parent = get_page(table->pager, parent_page_num);
    uint8_t* child = get_page(table->pager, child_page_num);
    uint32_t child_max_key = get_node_max_key(table->pager, child);
    uint32_t index = internal_node_find_child(parent, child_max_key);

    uint32_t parent_original_num_keys = *internal_node_num_keys(parent);

    if (parent_original_num_keys >= INTERNAL_NODE_MAX_KEYS) {
        internal_node_split_and_insert(table, parent_page_num, child_page_num);
        return;
    }

    uint32_t right_child_page_num = *internal_node_right_child_page_num(parent);
    if (right_child_page_num == INVALID_PAGE_NUM) {
        *internal_node_right_child_page_num(parent) = child_page_num;
        return;
    }
    uint8_t* right_child = get_page(table->pager, right_child_page_num);

    *internal_node_num_keys(parent) = parent_original_num_keys + 1;

    if (child_max_key > get_node_max_key(table->pager, right_child)) {
        *internal_node_child_page_num(parent, parent_original_num_keys) = right_child_page_num;
        *internal_node_key(parent, parent_original_num_keys) = get_node_max_key(table->pager, right_child);
        *internal_node_right_child_page_num(parent) = child_page_num;
    } else {
        for (uint32_t i = parent_original_num_keys; i > index; i--) {
            uint32_t* destination = internal_node_cell(parent, i);
            uint32_t* source = internal_node_cell(parent, i - 1);
            memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);
        }
        *internal_node_child_page_num(parent, index) = child_page_num;
        *internal_node_key(parent, index) = child_max_key;
    }
}

void internal_node_split_and_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    // old_node is the node that we will split
    uint32_t old_page_num = parent_page_num;
    uint8_t* old_node = get_page(table->pager, parent_page_num);
    uint32_t old_node_max_key = get_node_max_key(table->pager, old_node);

    uint8_t* child = get_page(table->pager, child_page_num); 
    uint32_t child_max_key = get_node_max_key(table->pager, child);

    uint32_t new_page_num = get_unused_page_num(table->pager);

    bool splitting_root = is_node_root(old_node);

    uint8_t* parent;
    uint8_t* new_node;
    if (splitting_root) {
        create_new_root(table, new_page_num);
        parent = get_page(table->pager, table->root_page_num);
        old_page_num = *internal_node_child_page_num(parent, 0);
        old_node = get_page(table->pager, old_page_num);
    } else {
        parent = get_page(table->pager, *node_parent_page_num(old_node));
        new_node = get_page(table->pager, new_page_num);
        initialize_internal_node(new_node);
    }
    
    uint32_t* old_node_num_keys = internal_node_num_keys(old_node);

    uint32_t cur_page_num = *internal_node_right_child_page_num(old_node);
    uint8_t* cur_node = get_page(table->pager, cur_page_num);

    internal_node_insert(table, new_page_num, cur_page_num);
    *node_parent_page_num(cur_node) = new_page_num;
    *internal_node_right_child_page_num(old_node) = INVALID_PAGE_NUM;

    for (int i = INTERNAL_NODE_MAX_KEYS - 1; i > INTERNAL_NODE_MAX_KEYS / 2; i--) {
        cur_page_num = *internal_node_child_page_num(old_node, i);
        cur_node = get_page(table->pager, cur_page_num);

        internal_node_insert(table, new_page_num, cur_page_num);
        *node_parent_page_num(cur_node) = new_page_num;

        (*old_node_num_keys)--;
    }

    *internal_node_right_child_page_num(old_node) = *internal_node_child_page_num(old_node, *old_node_num_keys - 1);
    (*old_node_num_keys)--;

    uint32_t max_after_split = get_node_max_key(table->pager, old_node);

    uint32_t destination_page_num = child_max_key < max_after_split ? old_page_num : new_page_num;

    internal_node_insert(table, destination_page_num, child_page_num);
    *node_parent_page_num(child) = destination_page_num;

    update_internal_node_key(parent, old_node_max_key, get_node_max_key(table->pager, old_node));

    if (!splitting_root) {
        internal_node_insert(table, *node_parent_page_num(old_node), new_page_num);
        *node_parent_page_num(new_node) = *node_parent_page_num(old_node);
    }
}

uint32_t* internal_node_num_keys(uint8_t* node) {
    return (uint32_t*)(node + INTERNAL_NODE_NUM_KEYS_OFFSET);
}

uint32_t* internal_node_right_child_page_num(uint8_t* node) {
    return (uint32_t*)(node + INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

uint32_t* internal_node_cell(uint8_t* node, uint32_t cell_num) {
    return (uint32_t*)(node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE);
}

uint32_t* internal_node_child_page_num(uint8_t* node, uint32_t child_num) {
    uint32_t num_keys = *internal_node_num_keys(node);
    if (child_num > num_keys) {
        printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
        exit(EXIT_FAILURE);
    } else if (child_num == num_keys) {
        uint32_t* right_child_page_num = internal_node_right_child_page_num(node);
        if (*right_child_page_num == INVALID_PAGE_NUM) {
            printf("Tried to access right child of node, but was invalid page\n");
            exit(EXIT_FAILURE);
        }
        return right_child_page_num;
    } else {
        // Internal node format: child (page num) | key
        uint32_t* child = internal_node_cell(node, child_num);
        if (*child == INVALID_PAGE_NUM) {
            printf("Tried to access child of node, but was invalid page\n");
            exit(EXIT_FAILURE);
        }
        return child;
    }
}

uint32_t* internal_node_key(uint8_t* node, uint32_t key_num) {
    return (uint32_t*)((uint8_t*)internal_node_cell(node, key_num) + INTERNAL_NODE_KEY_OFFSET);
}

void initialize_internal_node(uint8_t* node) {
    set_node_type(node, NODE_INTERNAL);
    set_node_root(node, false);
    *internal_node_num_keys(node) = 0;
    *internal_node_right_child_page_num(node) = INVALID_PAGE_NUM;
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

uint32_t get_node_max_key(Pager* pager, uint8_t* node) {
    if (get_node_type(node) == NODE_LEAF) {
        return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
    }

    uint8_t* right_child = get_page(pager, *internal_node_right_child_page_num(node));
    return get_node_max_key(pager, right_child);
}

void create_new_root(Table* table, uint32_t right_child_page_num) {
    uint8_t* root = get_page(table->pager, table->root_page_num);
    uint8_t* right_child = get_page(table->pager, right_child_page_num);

    // Will move the old root to the left child
    uint32_t left_child_page_num = get_unused_page_num(table->pager);
    uint8_t* left_child = get_page(table->pager, left_child_page_num);

    if (get_node_type(root) == NODE_INTERNAL) {
        initialize_internal_node(right_child);
        initialize_internal_node(left_child);
    }

    memcpy(left_child, root, PAGE_SIZE);
    set_node_root(left_child, false);

    // It depends on if the original root is a leaf or an internal node
    if (get_node_type(left_child) == NODE_INTERNAL) {
        uint8_t* child;
        for (int i = 0; i < *internal_node_num_keys(left_child); i++) {
            child = get_page(table->pager, *internal_node_child_page_num(left_child, i));
            *node_parent_page_num(child) = left_child_page_num;
        }
        child = get_page(table->pager, *internal_node_right_child_page_num(left_child));
        *node_parent_page_num(child) = left_child_page_num;
    }

    initialize_internal_node(root);
    set_node_root(root, true);
    *internal_node_num_keys(root) = 1;
    *internal_node_child_page_num(root, 0) = left_child_page_num;
    uint32_t left_child_max_key = get_node_max_key(table->pager, left_child);
    *internal_node_key(root, 0) = left_child_max_key;
    *internal_node_right_child_page_num(root) = right_child_page_num;
    *node_parent_page_num(left_child) = table->root_page_num;
    *node_parent_page_num(right_child) = table->root_page_num;
}
