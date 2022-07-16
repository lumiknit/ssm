#include "ssm_config.h"
#include "ssm_mem.h"

#include <stdint.h>
#include <stdlib.h>

/* Memory Pool */

typedef struct pool {
  size_t size;
  size_t left;
  val_t values[0];
} pool_t;

static pool_t* new_pool(size_t size) {
  pool_t *pool = (pool_t*) a_alloc(sizeof(val_t) * size);
  pool->size = size;
  pool->left = size;
  return pool;
}

#define del_pool(pool) a_free(pool)

/* mem_t Init/Del */

static void init_stack(mem_t *mem) {
  mem->stack_size = STACK_INIT_WORDS;
  mem->stack = (val_t*) a_alloc(sizeof(val_t) * mem->stack_size);
}

static void fin_stack(mem_t *mem) {
  a_free(mem->stack);
}

static void init_gc(mem_t *mem) {
  mem->minor = new_pool(MINOR_HEAP_WORDS);
  mem->major_head = NULL;
}

static void fin_gc(mem_t *mem) {
  del_pool(mem->minor);
  val_t *m = mem->major_head, *t;
  for(; m; m = t) {
    t = (val_t*) m[-1];
    a_free(m);
  }
}

mem_t* new_mem() {
  mem_t *mem = (mem_t*) malloc(sizeof(mem_t));
  init_code_set(&mem->code_set);
  init_stack(mem);
  init_gc(mem);
  return mem;
}

void del_mem(mem_t *mem) {
  fin_code_set(&mem->code_set);
  fin_stack(mem);
  fin_gc(mem);
  free(mem);
}