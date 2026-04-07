/*
 * memory.c — Virtual Heap Allocator
 *
 * First-Fit free list with 8-byte alignment and block splitting.
 * Permitted dependency: <stdio.h> for debug output only.
 */

#include "../include/memory.h"

static BlockHeader *heap_start = NULL;
static void        *heap_base  = NULL;
static size_t       heap_size  = 0;

void mem_init(void *raw_memory, size_t size)
{
    if (!raw_memory || size < MIN_BLOCK_SIZE) return;

    heap_base  = raw_memory;
    heap_size  = size;

    heap_start          = (BlockHeader *)raw_memory;
    heap_start->size    = size;
    heap_start->is_free = 1;
    heap_start->next    = NULL;
}

void *mem_alloc(size_t size)
{
    if (size == 0 || !heap_start) return NULL;

    size_t aligned_size = ALIGN(size);
    size_t total_needed = HEADER_SIZE + aligned_size;
    BlockHeader *cur    = heap_start;

    while (cur != NULL) {
        if (cur->is_free && cur->size >= total_needed) {
            size_t remaining = cur->size - total_needed;

            if (remaining >= MIN_BLOCK_SIZE) {
                BlockHeader *nb =
                    (BlockHeader *)((char *)cur + total_needed);
                nb->size    = remaining;
                nb->is_free = 1;
                nb->next    = cur->next;
                cur->size   = total_needed;
                cur->next   = nb;
            }

            cur->is_free = 0;
            return (void *)((char *)cur + HEADER_SIZE);
        }
        cur = cur->next;
    }

    return NULL; /* OOM */
}

void mem_free(void *ptr)
{
    if (!ptr || !heap_start) return;

    BlockHeader *block = (BlockHeader *)((char *)ptr - HEADER_SIZE);

    if ((char *)block < (char *)heap_base ||
        (char *)block >= (char *)heap_base + heap_size)
        return;

    if (block->is_free) return; /* double-free guard */

    block->is_free = 1;
    /* TODO: add forward coalescing to prevent fragmentation */
}

size_t mem_available(void)
{
    size_t total = 0;
    BlockHeader *cur = heap_start;

    while (cur != NULL) {
        if (cur->is_free && cur->size > HEADER_SIZE)
            total += cur->size - HEADER_SIZE;
        cur = cur->next;
    }

    return total;
}
