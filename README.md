# simple-sqlite
Reference: https://cstack.github.io/db_tutorial/

## Build
```
>> mkdir build && cd build
>> cmake -DCMAKE_BUILD_TYPE=Debug ..
>> make
>> ./simpleSQLite
```

## Test
### Basic
```
db > insert 1 ben ben@gmail.com
Executed.

db > insert 2 tom tom@yahoo.com
Executed.

db > select
(1, ben, ben@gmail.com)
(2, tom, tom@yahoo.com)
Executed.
```

### Test with RSpec
```
>> bundle init
```

Add `gem 'rspec'` in the file Gemfile, then run `bundle install`:
```
>> bundle install
```

Create a new file `spec/db_test_spec.rb` and run the test:
```
>> bundle exec rspec
```

## Project Related
### Structure
Basic Steps:
Read Input -> Execute Meta Command -> Prepare Statement -> Execute Statement

### Parse Input
Version 1:
```
int args_assigned = sscanf(
    input_buffer->buffer, 
    "insert %d %s %s", 
    &(statement->row_to_insert.id),
    statement->row_to_insert.username, 
    statement->row_to_insert.email
);
```

Version 2:
```
char* keyword = strtok(input_buffer->buffer, " ");
char* id_string = strtok(NULL, " ");
char* username = strtok(NULL, " ");
char* email = strtok(NULL, " ");
```

### Layout
#### Common Node Header Layout
NODE TYPE | IS ROOT | PARENT POINTER

#### Leaf Node Header Layout
NODE TYPE | IS ROOT | PARENT POINTER | LEAF NODE NUM CELLS | LEAF_NODE_NEXT_LEAF

#### Leaf Node Body Layout
LEAF NODE KEY | LEAF NODE VALUE

#### Internal Node Header Layout
NODE TYPE | IS ROOT | PARENT POINTER | INTERNAL NODE NUM KEYS | INTERNAL_NODE_RIGHT_CHILD

#### Internal Node Body Layout
INTERNAL_NODE_CHILD | INTERNAL_NODE_KEY


### Cursor Design
#### Before Introducing B-Tree
```
typedef struct {
    Table* table;
    uint32_t row_num;
    bool end_of_table;  // Indicates a position one past the last element
} Cursor;

Cursor* table_start(Table* table) {
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->row_num = 0;
    cursor->end_of_table = (table->num_rows == 0);

    return cursor;
}

Cursor* table_end(Table* table) {
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->row_num = table->num_rows;
    cursor->end_of_table = true;

    return cursor;
}

uint8_t* cursor_value(Cursor* cursor) {
    uint32_t row_num = cursor->row_num;
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    uint8_t* page = get_page(cursor->table->pager, page_num);
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;

    return page + byte_offset;
}

void cursor_advance(Cursor* cursor) {
    cursor->row_num += 1;
    if (cursor->row_num >= cursor->table->num_rows) {
        cursor->end_of_table = true;
    }
}
```

### Page Num
Each node is also a page, it has a page number (uint32_t).  
When we talk about a node (root node, internal node, leaf node, child node), we use page number to identify it.

For example:
```
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
```
The response of this function is uint32_t*, which is the address of the child info inside current internal node. The child info is actually just a page number.

## C Knowledge
### ssize_t
ssize_t is a data type commonly used in programming to represent sizes or counts of bytes. It stands for "signed size type". It's often used when dealing with functions or operations that involve reading or writing data, such as file I/O or memory operations.

The key feature of ssize_t is that it's a signed integer type, which means it can represent both positive and negative values. This is important because functions that return sizes or counts might use negative values to indicate errors or special conditions.

For example, in C programming, when you're working with functions like read() or write() for reading from or writing to files or sockets, the return type is often ssize_t. Similarly, the fread() and fwrite() functions for reading and writing data from and to streams also use ssize_t to indicate the number of bytes read or written.

### Calculate the size of a struct
```
sizeof(((Struct*)0)->Attribute)
```

### How to handle command input
```
typedef struct {
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

InputBuffer* new_input_buffer() {
    InputBuffer* input_buffer = (InputBuffer*) malloc(sizeof(InputBuffer));

    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}
```

### input_length and buffer_length
input_length is the number of bytes that the user has typed into the console. buffer_length is the size of the allocated buffer.

Generally speaking, the buffer_length is power of 2. For example, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, etc.

### lseek
The lseek() function in C is used to reposition the file offset associated with a file descriptor. The syntax is:
```
#include <unistd.h>

off_t lseek(int fildes, off_t offset, int whence);
```

where:
- fildes is the file descriptor of the file to be repositioned
- offset is the new file offset
- whence is a flag indicating how the offset is interpreted

The whence flag can be one of the following values:
- SEEK_SET: The offset is interpreted as an absolute offset from the beginning of the file.
- SEEK_CUR: The offset is interpreted as an offset from the current file position.
- SEEK_END: The offset is interpreted as an offset from the end of the file.

### void
In GCC, sizeof(void) is 1. So, if we increment a void pointer, it will be incremented by 1 byte.  
However, I prefer uint8_t* to void*, which is more platform-independent.

In the project, uint8_t* refer to the address of the node, uint32_t refer to the page number of the node, and  
uint32_t* refer to the cell inside a node.

## DB Knowledge
### SQList Architecture
#### Architecture
Tokenizer -> Parser -> Code Generator -> Virtual Machine -> B-Tree -> Pager -> OS Interface

#### Front end
Tokenizer -> Parser -> Code Generator

#### Back end:  
Virtual Machine -> B-Tree -> Pager -> OS Interface

#### Virtual Machine
The virtual machine takes bytecode generated by the front-end as instructions. It can then perform operations on one or more tables or indexes, each of which is stored in a data structure called a B-tree. The VM is essentially a big switch statement on the type of bytecode instruction.

#### Pager (Buffer)
The pager receives commands to read or write pages of data. It is responsible for reading/writing at appropriate offsets in the database file. It also keeps a cache of recently-accessed pages in memory, and determines when those pages need to be written back to disk.

### Difference between B-Tree (B Tree) and B+Tree (B Plus Tree)
Because B+ trees don't have data associated with interior nodes, more keys can fit on a page of memory. Therefore, it will require fewer cache misses in order to access data that is on a leaf node.

The leaf nodes of B+ trees are linked, so doing a full scan of all objects in a tree requires just one linear pass through all the leaf nodes. A B tree, on the other hand, would require a traversal of every level in the tree. This full-tree traversal will likely involve more cache misses than the linear traversal of B+ leaves.

## Others
### Vim Trick
__:%!xxd__: Convert the file to hex format.

- :% applies the following command to the entire file.
- ! indicates that you're running an external command.
- xxd is an external command-line utility used to create a hexadecimal dump of a file or standard input.
