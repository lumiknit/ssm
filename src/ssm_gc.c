/**
 * @file ssm_gc.c
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#include <ssm_i.h>

void initMem(Mem* mem,
  size_t minor_heap_size,
  size_t major_gc_threshold_percent,
  size_t stack_size,
  size_t global_size) {
  memset(mem, 0, sizeof(Mem));

  mem->minor = newStack(minor_heap_size);
  // Minor heap's top = left
  mem->minor->top = minor_heap_size;

  mem->major_gc_threshold_percent = major_gc_threshold_percent;
  mem->stack = newStack(stack_size);
  mem->global = newStack(global_size);
}

void finiMem(Mem* mem) {
  // Free minor heap
  free(mem->minor);
  // Free major heap
  ssmT majors[] = {mem->major_immortal, mem->major_leaves, mem->major_nodes};
  int m;
  ssmT i, t;
  for(m = 0; m < 3; m++) {
    for(i = majors[m]; i != NULL; i = t) {
      t = ssmTNext(i);
      free(i);
    }
  }
  // Free stacks
  freeStack(mem->stack);
  freeStack(mem->global);
}

static inline ssmT allocMajorLong(Mem* mem, ssmV bytes) {
  const size_t size = ssmTWordsFromBytes(bytes);
  void* p = aligned_alloc(SSM_WORD_SIZE, SSM_WORD_SIZE + size);
  if(p == NULL) {
    panicf("Failed to alloc major long of size %zu", bytes);
  }
  ssmT tup = ((ssmT)p) + 1;
  // Write major info
  ssmTNext(tup) = mem->major_leaves;
  mem->major_leaves = tup;
  mem->major_allocated_words += size;
  ssmTHd(tup) = ssmLongHd(bytes);
  return tup;
}

static inline ssmT allocMajorShort(Mem* mem, ssmV tag, ssmV words) {
  const size_t size = ssmTWords(words);
  void* p = aligned_alloc(SSM_WORD_SIZE, SSM_WORD_SIZE + size);
  if(p == NULL) {
    panicf("Failed to alloc major short of size %zu", words);
  }
  ssmT tup = ((ssmT)p) + 1;
  // Write major info
  ssmTNext(tup) = mem->major_nodes;
  mem->major_nodes = tup;
  mem->major_allocated_words += size;
  ssmTHd(tup) = ssmShortHd(tag, words);
  return tup;
}

void fullGC(Mem* mem) {
  unimplemented();
  mem->major_gc_count++;
}

void minorGC(Mem* mem) {
  unimplemented();
  mem->minor_gc_count++;
}

ssmT newLong(Mem *mem, ssmV bytes) {
  unimplemented();
}

ssmT newTup(Mem *mem, ssmV tag, ssmV words) {
  unimplemented();
}