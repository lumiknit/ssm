/**
 * @file ssm_i.h
 * @author lumiknit (aasr4r4@gmail.com)
 * @brief ssm internal header
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#ifndef __SSM_I_H__
#define __SSM_I_H__

#include <ssm.h>

// Helpers
#define panic(msg) { \
  fprintf(stderr, "Panic at %s:%d: %s\n", __FILE__, __LINE__, msg); \
  exit(1); \
}
#define panicf(format, ...) { \
  fprintf(stderr, "Panic at %s:%d: " format "\n", \
    __FILE__, __LINE__, __VA_ARGS__); \
  exit(1); \
}
#define unimplemented() { \
  fprintf(stderr, "Unimplemented! %s:%d\n", __FILE__, __LINE__); \
  exit(1); \
}

// Stack
typedef struct Stack {
  size_t size;
  size_t top;
  ssmV data[1];
} Stack;

Stack* newStack(size_t size);
#define freeStack(stack) free(stack)
Stack* extendStack(Stack* stack, size_t size);
size_t pushStack(Stack* stack, ssmV value);
void pushStackForce(Stack** stack, ssmV value);
ssmV popStack(Stack* stack);

// GC

typedef struct Mem {
  // Configurations
  size_t major_gc_threshold_percent;

  // Minor heap
  Stack* minor;

  // Major heap
  size_t major_allocated_words;
  size_t major_gc_threshold;
  // Immortal objects (never freed, e.g. global constant)
  ssmV* major_immortal;
  // Leaves (cannot contain any reference, e.g. long tuple)
  ssmV* major_leaves;
  // Nodes (may contain references)
  ssmV* major_nodes;
  
  // Stack
  Stack* stack;
  Stack* global;

  // Statistics
  size_t minor_gc_count;
  size_t major_gc_count;
} Mem;

void initMem(Mem* mem,
  size_t minor_heap_size,
  size_t major_gc_threshold_percent,
  size_t stack_size,
  size_t global_size);
void finiMem(Mem* mem);

void fullGC(Mem* mem);
void minorGC(Mem* mem);

ssmT newLong(Mem *mem, ssmV bytes);
ssmT newTup(Mem *mem, ssmV tag, ssmV words);

#endif