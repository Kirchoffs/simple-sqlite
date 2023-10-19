#ifndef STATS_H
#define STATS_H

#include <stdint.h>
#include "table.h"

void print_constants(void);

void print_node_constants(void);

void indent(uint32_t level);

void print_tree(Pager* pager, uint32_t page_num);

void print_tree_with_level(Pager* pager, uint32_t page_num, uint32_t indentation_level);

#endif
