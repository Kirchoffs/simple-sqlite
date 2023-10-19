#include "cursor.h"
#include "node.h"
#include <stdlib.h>
#include <stdio.h>

uint8_t* cursor_value(Cursor* cursor) {
    uint8_t* page = get_page(cursor->table->pager, cursor->page_num);
    return leaf_node_value(page, cursor->cell_num);
}

void cursor_advance(Cursor* cursor) {
    uint32_t page_num = cursor->page_num;
    uint8_t* node = get_page(cursor->table->pager, page_num);
    cursor->cell_num += 1;
    if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
        cursor->end_of_table = true;
    }
}

Cursor* table_start(Table* table) {
    Cursor* cursor = (Cursor*) malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = table->root_page_num;
    cursor->cell_num = 0;

    uint8_t* root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->end_of_table = (num_cells == 0);

    return cursor;
}

Cursor* table_find(Table* table, uint32_t key) {
    uint32_t root_page_num = table->root_page_num;
    uint8_t* root_node = get_page(table->pager, root_page_num);

    if (get_node_type(root_node) == NODE_LEAF) {
        return leaf_node_find(table, root_page_num, key);
    } else {
        return internal_node_find(table, root_page_num, key);
    }
}

Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
    uint8_t* node = get_page(table->pager, page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = page_num;

    uint32_t l_index = 0;
    uint32_t r_index = num_cells;
    while (l_index < r_index) {
        uint32_t index = l_index + (r_index - l_index) / 2;
        uint32_t key_at_index = *leaf_node_key(node, index);

        if (key_at_index < key) {
            l_index = index + 1;
        } else {
            r_index = index;
        }
    }

    cursor->cell_num = l_index;
    return cursor;
}

Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key) {
    uint8_t* node = get_page(table->pager, page_num);
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

    uint32_t child_page_num = *internal_node_child(node, l_index);
    uint8_t* child_page = get_page(table->pager, child_page_num);
    switch (get_node_type(child_page)) {
        case NODE_LEAF:
            return leaf_node_find(table, child_page_num, key);
        case NODE_INTERNAL:
            return internal_node_find(table, child_page_num, key);
    }
}
