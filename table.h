#ifndef TABLE_H
#define TABLE_H

#include <stdint.h>
#include "row.h"

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    uint8_t* pages[400];
} Pager;

typedef struct {
    uint32_t root_page_num;
    Pager* pager;
} Table;

uint8_t* get_page(Pager* pager, uint32_t page_num);
Pager* pager_open(const char* filename);
void pager_flush(Pager* pager, uint32_t page_num);
Table* db_open(const char* filename);
void db_close(Table* table);

#endif
