#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>

#define MIN_BLOCK_SIZE 16
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define HEADER_SIZE (sizeof(Block))

void *malloc(size_t size);
void free(void *ptr);

#endif
