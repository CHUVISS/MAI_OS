#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MIN_BLOCK_SIZE 16

int log2s(size_t n) {
    if (n == 0) {
        return -1;
    }
    int result = 0;
    while (n > 1) {
        n >>= 1;
        result++;
    }
    return result;
}

typedef struct BlockHeader {
    struct BlockHeader *next;
} BlockHeader;

typedef struct Allocator {
    BlockHeader **free_lists;
    size_t num_lists;
    void *base_addr;
    size_t total_size;
} Allocator;

Allocator *allocator_create(void *memory, size_t size) {
    if (!memory || size < sizeof(Allocator)) {
        return NULL;
    }
    Allocator *allocator = (Allocator *)memory;
    allocator->base_addr = memory;
    allocator->total_size = size;

    size_t max_block_size = size;

    allocator->num_lists = (size_t)(log2s(max_block_size) - log2s(MIN_BLOCK_SIZE) + 1);
    allocator->free_lists =
        (BlockHeader **)((char *)memory + sizeof(Allocator));

    for (size_t i = 0; i < allocator->num_lists; i++) {
        allocator->free_lists[i] = NULL;
    }

    void *current_block = (char *)memory + sizeof(Allocator) +
                          allocator->num_lists * sizeof(BlockHeader *);
    size_t remaining_size =
        size - sizeof(Allocator) - allocator->num_lists * sizeof(BlockHeader *);

    size_t block_size = MIN_BLOCK_SIZE;
    while (remaining_size >= block_size) {
        BlockHeader *header = (BlockHeader *)current_block;
        size_t index = log2s(block_size) - log2s(MIN_BLOCK_SIZE);
        header->next = allocator->free_lists[index];
        allocator->free_lists[index] = header;

        current_block = (char *)current_block + block_size;
        remaining_size -= block_size;
        block_size <<= 1;
    }
    return allocator;
}

void *allocator_alloc(Allocator *allocator, size_t size) {
    if (!allocator || size == 0) {
        return NULL;
    }

    size_t adjusted_size = size < MIN_BLOCK_SIZE ? MIN_BLOCK_SIZE : size;
    size_t index = log2s(adjusted_size) - log2s(MIN_BLOCK_SIZE);

    if (index >= allocator->num_lists) {
        return NULL;
    }

    for (size_t i = index; i < allocator->num_lists; i++) {
        if (allocator->free_lists[i] != NULL) {
            BlockHeader *block = allocator->free_lists[i];
            allocator->free_lists[i] = block->next;

            size_t block_size = MIN_BLOCK_SIZE << i;
            while (i > index) {
                i--;
                block_size >>= 1;

                BlockHeader *split_block =
                    (BlockHeader *)((char *)block + block_size);
                split_block->next = allocator->free_lists[i];
                allocator->free_lists[i] = split_block;
            }

            return (void *)((char *)block + sizeof(BlockHeader));
        }
    }

    return NULL;
}

void allocator_free(Allocator *allocator, void *ptr) {
    if (!allocator || !ptr) {
        return;
    }

    BlockHeader *block = (BlockHeader *)((char *)ptr - sizeof(BlockHeader));
    size_t block_offset = (char *)block - (char *)allocator->base_addr;
    size_t block_size = MIN_BLOCK_SIZE;

    while (block_offset % (block_size * 2) == 0 && block_size < allocator->total_size) {
        size_t buddy_offset = block_offset ^ block_size;
        BlockHeader *buddy = (BlockHeader *)((char *)allocator->base_addr + buddy_offset);
        size_t buddy_index = log2s(block_size) - log2s(MIN_BLOCK_SIZE);


        BlockHeader *prev = NULL;
        BlockHeader *curr = allocator->free_lists[buddy_index];
        while (curr) {
            if (curr == buddy) {
                if (prev) {
                    prev->next = curr->next;
                } else {
                    allocator->free_lists[buddy_index] = curr->next;
                }
                block_offset &= ~(block_size * 2 - 1);
                block = (BlockHeader *)((char *)allocator->base_addr + block_offset);
                block_size *= 2;
                goto continue_coalescing;
            }
            prev = curr;
            curr = curr->next;
        }
        break;

    continue_coalescing:;
    }

    size_t index = log2s(block_size) - log2s(MIN_BLOCK_SIZE);
    block->next = allocator->free_lists[index];
    allocator->free_lists[index] = block;
}

void allocator_destroy(Allocator *allocator) {
    if (allocator) {
        munmap(allocator->base_addr, allocator->total_size);
    }
}

