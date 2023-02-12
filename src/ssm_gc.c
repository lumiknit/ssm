/**
 * @file ssm_gc.c
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#include <ssm_i.h>

static void updateMajorGCThreshold(Mem* mem) {
  // If major_gc_threshold_percent is 0, disable major GC
  if(mem->major_gc_threshold_percent == 0) {
    mem->major_gc_threshold = SIZE_MAX;
    return;
  }
  // Set major gc thresold to minimum major heap size
  mem->major_gc_threshold = mem->minor->size * 7;
  // To calculate percentage, split allocated into two part
  const size_t lo = mem->major_allocated_words % 100;
  const size_t hi = mem->major_allocated_words / 100;
  // Calculate percentage with checking overflow
  const size_t lo_percented = lo * mem->major_gc_threshold_percent / 100;
  size_t hi_percented = hi * mem->major_gc_threshold_percent;
  if(hi_percented / mem->major_gc_threshold_percent != hi) {
    // Overflowed
    hi_percented = SIZE_MAX;
  }
  size_t percented;
  if(hi_percented > SIZE_MAX - lo_percented) {
    // Overflow
    percented = SIZE_MAX;
  } else {
    percented = hi_percented + lo_percented;
  }
  // Update if calculated one is larger
  if(percented > mem->major_gc_threshold) {
    mem->major_gc_threshold = percented;
  }
}

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

  updateMajorGCThreshold(mem);
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

typedef int (*MarkableFn)(Mem*, ssmT);

static void markVal(Mem *mem, ssmV val, MarkableFn markable) {
  if(!ssmIsGCVal(val)) return;
  ssmT tup = ssmVal2Tup(val);
  if(!markable(mem, tup)) return;
  const ssmV hd = ssmTHd(tup);
  if(ssmHdColor(hd)) return;
  ssmTHd(tup) = ssmHdMarked(hd);
  if(ssmHdIsLong(hd)) return;
  pushStackForce(&mem->mark_stack, val);
  while(mem->mark_stack->top > 0) {
    ssmT marked_tup = ssmVal2Tup(popStack(mem->mark_stack));
    const ssmV words = ssmHdShortWords(ssmTHd(marked_tup));
    ssmV i;
    for(i = 0; i < words; i++) {
      if(!ssmIsGCVal(ssmTElem(marked_tup, i))) continue;
      ssmT tup = ssmVal2Tup(ssmTElem(marked_tup, i));
      if(!markable(mem, tup)) continue;
      const ssmV hd = ssmTHd(tup);
      if(ssmHdColor(hd)) continue;
      ssmTHd(tup) = ssmHdMarked(hd);
      if(ssmHdIsLong(hd)) return;
      pushStackForce(&mem->mark_stack, ssmTup2Val(tup));
    }
  }
}

static void markPhase(Mem* mem, MarkableFn markable) {
  ssmV i;
  // Mark global stack
  for(i = 0; i < mem->global->top; i++) {
    markVal(mem, mem->global->vals[i], markable);
  }
  // Mark stack
  for(i = 0; i < mem->stack->top; i++) {
    markVal(mem, mem->stack->vals[i], markable);
  }
}

static int markableMajor(Mem* mem, ssmT tup) {
  (void) mem;
  return tup != NULL;
}

static int markableMinor(Mem* mem, ssmT tup) {
  return inStack(mem->minor, tup);
}

static void freeUnmarkedMajor(Mem* mem) {
  unimplemented();
}

static void moveMinorToMajor(Mem* mem) {
  unimplemented();
}

int fullGC(Mem* mem) {
  // Run marking phase
  markPhase(mem, markableMajor);
  // Traverse object list and free unmarked objects
  // also unmark all marked objects
  freeUnmarkedMajor(mem);
  // TODO: Move marked object in minor heap to major heap
  moveMinorToMajor(mem);
  // Rewind minor heap's top pointer (left)
  mem->minor->top = mem->minor->size;
  // Adjust major gc threshold
  updateMajorGCThreshold(mem);
  // Update statistics
  mem->major_gc_count++;
  return 0;
}

int minorGC(Mem* mem) {
  // Check major heap is full
  size_t major_allocated_guess = mem->major_allocated_words;
  if(major_allocated_guess > SIZE_MAX - mem->minor->size) {
    // Overflowed
    major_allocated_guess = SIZE_MAX;
  } else {
    major_allocated_guess += mem->minor->size;
  }
  if(major_allocated_guess >= mem->major_gc_threshold) {
    // Major heap is full, do full GC
    return fullGC(mem);
  }
  // Run marking phase
  markPhase(mem, markableMinor);
  // Move marked objects to major heap
  moveMinorToMajor(mem);
  // Rewind minor heap's top pointer (left)
  mem->minor->top = mem->minor->size;
  // Update statistics
  mem->minor_gc_count++;
  return 0;
}

ssmT newLongTup(Mem *mem, ssmV bytes) {
  // Get size
  const size_t size = ssmTWordsFromBytes(bytes);
  if(mem->minor->size < size) {
    // Minor heap is not enough, just alloc in major
    return allocMajorLong(mem, bytes);
  } else if(mem->minor->top < size) {
    // Minor heap is enough, but not enough space
    // Do minor GC
    minorGC(mem);
  }
  // Move top
  mem->minor->top -= size;
  ssmT tup = ((ssmT)mem->minor->vals) + mem->minor->top;
  // Initialize tuple header
  ssmTHd(tup) = ssmLongHd(bytes);
  return tup;
}

ssmT newTup(Mem *mem, ssmV tag, ssmV words) {
  // Get size
  const size_t size = ssmTWords(words);
  if(mem->minor->size < size) {
    // Minor heap is not enough
    // In this case, first run minor GC for invariant
    if(minorGC(mem) < 0) {
      panic("Minor GC failed");
    }
    return allocMajorShort(mem, tag, words);
  } 
  if(mem->minor->top < size) {
    // Minor heap is enough, but not enough space
    // Do minor GC
    minorGC(mem);
  }
  // Move top
  mem->minor->top -= size;
  ssmT tup = ((ssmT)mem->minor->vals) + mem->minor->top;
  // Initialize tuple header
  ssmTHd(tup) = ssmShortHd(tag, words);
  return tup;
}