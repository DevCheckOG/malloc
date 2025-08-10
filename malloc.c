#include <unistd.h>
#include <sys/mman.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MIN_BLOCK_SIZE 16
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define HEADER_SIZE (sizeof(Block))

typedef struct Block {
    size_t size;
    struct Block *next;
    int free;
} Block;

static Block *free_list = NULL;
static Block *heap_start = NULL;
static size_t heap_size = 0;

static Block *find_free_block(size_t size) {
    Block *current = free_list;

    while (current) {
        if (current->free && current->size >= size) return current;
        current = current->next;
    }

    return NULL;
}

static Block *request_space(size_t size) {
    size_t total_size = size + HEADER_SIZE;
    void *new_block = mmap(NULL, total_size, PROT_READ | PROT_WRITE, 
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (new_block == MAP_FAILED) return NULL;

    Block *block = (Block *)new_block;
    block->size = size;
    block->free = 0;
    block->next = NULL;

    if (!heap_start) heap_start = block;

    heap_size += total_size;

    return block;
}

static void split_block(Block *block, size_t size) {
    if (block->size >= size + HEADER_SIZE + MIN_BLOCK_SIZE) {
        Block *new_block = (Block *)((char *)block + HEADER_SIZE + size);
        new_block->size = block->size - size - HEADER_SIZE;
        new_block->free = 1;
        new_block->next = block->next;

        block->size = size;
        block->next = new_block;

        if (free_list == block) free_list = new_block;
    }
}

static void destroy() {
    Block *current = heap_start;

    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += HEADER_SIZE + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void *malloc(size_t size) {
    if (size == 0) return NULL;

    size = ALIGN(size);

    Block *block = find_free_block(size);
    if (block) {
        block->free = 0;
        split_block(block, size);
        return (void *)((char *)block + HEADER_SIZE);
    }

    block = request_space(size);

    if (!block) return NULL;

    return (void *)((char *)block + HEADER_SIZE);
}

void free(void *ptr) {
    if (!ptr) return;

    Block *block = (Block *)((char *)ptr - HEADER_SIZE);
    block->free = 1;

    if (!free_list || block < free_list) {
        block->next = free_list;
        free_list = block;
    } else {
        Block *current = free_list;

        while (current->next && current->next < block) {
            current = current->next;
        }

        block->next = current->next;
        current->next = block;
    }

    destroy();
}
