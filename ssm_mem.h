#ifndef __SSM_MEM_H__
#define __SSM_MEM_H__

#include <stdlib.h>

#include "ssm_val.h"
#include "ssm_code.h"

/* Memory Context */

struct pool;

typedef struct mem {
  /* Codes */
  code_set_t code_set;
    
  /* Stack */
  size_t stack_size;
  val_t *stack;

  /* -- GC -- */
  struct pool *minor;
  val_t *major_head;
} mem_t;

mem_t* new_mem();
void del_mem(mem_t*);

/* Global Memory Helpers */

op_t* register_code(mem_t*, code_set_t*);

size_t allocated_globals(mem_t*);
val_t* alloc_global(mem_t*, size_t size);
val_t* get_global(mem_t*, size_t index);

/* Stack Helpers */

val_t* check_and_extend_stack(mem_t*, val_t *sp);

/* GC */

val_t* alloc_small(mem_t*, uint8_t tag, size_t size);
val_t* alloc_large(mem_t*, size_t size);

void run_gc();

#endif
