#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

extern const uint32_t TABLE_MAX_PAGES;
extern const uint32_t PAGE_SIZE;

extern const uint32_t ID_SIZE;
extern const uint32_t USERNAME_SIZE;
extern const uint32_t EMAIL_SIZE;
extern const uint32_t ID_OFFSET;
extern const uint32_t USERNAME_OFFSET;
extern const uint32_t EMAIL_OFFSET;
extern const uint32_t ROW_SIZE;

extern const uint32_t COLUMN_USERNAME_LENGTH;
extern const uint32_t COLUMN_EMAIL_LENGTH;

/**
 * 
 * Common Node Header Layout
 * 
 */

extern const uint32_t NODE_TYPE_SIZE;
extern const uint32_t NODE_TYPE_OFFSET;
extern const uint32_t IS_ROOT_SIZE;
extern const uint32_t IS_ROOT_OFFSET;
extern const uint32_t PARENT_POINTER_SIZE;
extern const uint32_t PARENT_POINTER_OFFSET;
extern const uint8_t COMMON_NODE_HEADER_SIZE;

/**
 * 
 * Leaf Node Header Layout
 * 
 */

extern const uint32_t LEAF_NODE_NUM_CELLS_SIZE;
extern const uint32_t LEAF_NODE_NUM_CELLS_OFFSET;
const uint32_t LEAF_NODE_NEXT_LEAF_SIZE;
const uint32_t LEAF_NODE_NEXT_LEAF_OFFSET;
extern const uint32_t LEAF_NODE_HEADER_SIZE;

/**
 *
 * Leaf Node Body Layout
 *  
 */

extern const uint32_t LEAF_NODE_KEY_SIZE;
extern const uint32_t LEAF_NODE_KEY_OFFSET;
extern const uint32_t LEAF_NODE_VALUE_SIZE;
extern const uint32_t LEAF_NODE_VALUE_OFFSET;
extern const uint32_t LEAF_NODE_CELL_SIZE;
extern const uint32_t LEAF_NODE_SPACE_FOR_CELLS;
extern const uint32_t LEAF_NODE_MAX_CELLS;
const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT;
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT;

/**
 * 
 * Internal Node Header Layout
 * 
 */

const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET;
const uint32_t INTERNAL_NODE_HEADER_SIZE;

/**
 * 
 * Internal Node Body Layout
 * 
 */

const uint32_t INTERNAL_NODE_CHILD_SIZE;
const uint32_t INTERNAL_NODE_CHILD_OFFSET;
const uint32_t INTERNAL_NODE_KEY_SIZE;
const uint32_t INTERNAL_NODE_KEY_OFFSET;
const uint32_t INTERNAL_NODE_CELL_SIZE;
const uint32_t INTERNAL_NODE_MAX_KEYS;

#endif
