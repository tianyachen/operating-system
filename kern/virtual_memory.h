#ifndef _VIRTUAL_MEMORY_H
#define _VIRTUAL_MEMORY_H

typedef uint32_t page_table_entry_t;

typedef struct page_directory {
    uint32_t *page_directory_vector;
    // mutex

} page_directory_t



#endif /* _VIRTUAL_MEMORY_H */