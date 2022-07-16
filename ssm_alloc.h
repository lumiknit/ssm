#ifndef __SSM_ALLOC_H__
#define __SSM_ALLOC_H__

#include <stdlib.h>

/* Aligned Allocator */

void* a_alloc(size_t size);
void a_free(void *ptr);
void* a_realloc(void *ptr, size_t new_size);

#endif