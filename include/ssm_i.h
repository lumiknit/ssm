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
  fprintf(stderr, "[FATAL] (ssm) Panic at %s:%d: %s\n", \
    __FILE__, __LINE__, msg); \
  exit(1); \
}
#define panicf(format, ...) { \
  fprintf(stderr, "[FATAL] (ssm) Panic at %s:%d: " format "\n", \
    __FILE__, __LINE__, __VA_ARGS__); \
  exit(1); \
}
#define unimplemented() { \
  fprintf(stderr, "[FATAL] (ssm) Unimplemented! %s:%d\n", \
    __FILE__, __LINE__); \
  exit(1); \
}

// Logging helper
#define DEBUG_LOG_ENABLED

#ifdef DEBUG_LOG_ENABLED
#define logf(...) printf(__VA_ARGS__)
#else
#define logf(...) ((void)0)
#endif


// Stack
typedef struct Stack {
  size_t size;
  size_t top;
  ssmV vals[1];
} Stack;

// If r2l is true, the sp moves from right to left (as call stack)
// and must use pushStackR and popStackR
Stack* newStack(size_t size, int r2l);
#define freeStack(stack) free(stack)
// Note: extendStack does not guarantee the stack is aligned
Stack* extendStackToRight(Stack* stack, size_t size);
Stack* extendStackToLeft(Stack* stack, size_t size);
size_t pushStack(Stack* stack, ssmV value);
void pushStackForce(Stack** stack, ssmV value);
ssmV popStack(Stack* stack);
size_t pushStackR(Stack* stack, ssmV value);
ssmV popStackR(Stack* stack);
#define inStack(stack, ptr) \
  ((stack)->vals <= (ptr) && \
  (ptr) <= (stack)->vals + (stack)->size - 1)

// GC

#define MIN_MAJOR_HEAP_FACTOR 7

typedef struct Mem {
  // Configurations
  size_t major_gc_threshold_percent;

  // Minor heap
  Stack* minor;

  // Major heap
  size_t major_allocated_words;
  size_t major_gc_threshold_words;

  // Major tuple list
#define MAJOR_LIST_KINDS 3
#define MAJOR_LIST_IMMORTAL 0 // Never freed
#define MAJOR_LIST_LEAVES 1 // Cannot contain any reference
#define MAJOR_LIST_NODES 2 // May contain references
  ssmV *major_list[3];
  
  // Stack
  Stack* stack;
  Stack* global;

  // Statistics
  size_t minor_gc_count;
  size_t major_gc_count;

  // GC template variables
  ssmT mark_list;
} Mem;

void initMem(Mem* mem,
  size_t minor_heap_words,
  size_t major_gc_threshold_percent,
  size_t stack_size,
  size_t global_size);
void finiMem(Mem* mem);

void updateMajorGCThreshold(Mem* mem);

int fullGC(Mem* mem);
int minorGC(Mem* mem);

ssmT newLongTup(Mem *mem, ssmV bytes);
ssmT newTup(Mem *mem, ssmV tag, ssmV words);

#endif