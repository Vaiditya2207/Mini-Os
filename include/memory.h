#ifndef MEMORY_H
#define MEMORY_H

/*
 * memory.h — Virtual Heap Allocator Interface
 *
 * First-Fit Free List allocator over a fixed memory region.
 * Simulates OS heap management without malloc/free.
 */

#include <stddef.h>

typedef struct BlockHeader {
    size_t              size;     /* total block size including header */
    int                 is_free;
    struct BlockHeader *next;
} BlockHeader;

#define VIRTUAL_RAM_SIZE   (64 * 1024)
#define ALIGNMENT          8
#define HEADER_SIZE        (sizeof(BlockHeader))
#define MIN_BLOCK_SIZE     (HEADER_SIZE + ALIGNMENT)
#define ALIGN(size)        (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

void   mem_init(void *raw_memory, size_t size);
void  *mem_alloc(size_t size);
void   mem_free(void *ptr);
size_t mem_available(void);

#endif /* MEMORY_H */
