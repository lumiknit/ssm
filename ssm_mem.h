#ifndef __SSM_MEM_H__
#define __SSM_MEM_H__

#include <stdlib.h>

#include "ssm_val.h"

/* Aligned Allocator */

void* a_alloc(size_t size);
void a_free(void *ptr);
void* a_realloc(void *ptr, size_t new_size);

/* Global Memory Helpers */

op_t* alloc_code(size_t size);

size_t allocated_globals();
val_t* alloc_global(size_t size);
val_t* get_global(size_t index);

/* Stack Helpers */

val_t* init_stack();
val_t* stack_sp();
val_t* stack_bp();

val_t* check_and_extend_stack(val_t *sp);

/* GC */

void init_gc();

val_t* alloc_small(uint8_t tag, size_t size);
val_t* alloc_large(size_t size);

void run_gc();

#endif
