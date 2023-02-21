/**
 * @file ssm_gc.c
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#include <ssm_i.h>

void updateMajorGCThreshold(Mem* mem) {
  // If major_gc_threshold_percent is 0, disable major GC
  if(mem->major_gc_threshold_percent == 0) {
    mem->major_gc_threshold_words = SIZE_MAX;
    return;
  }
  // Calculate new major gc threshold percent
  size_t percent = mem->major_gc_threshold_percent + 100;
  if(percent < mem->major_gc_threshold_percent) {
    // Overflow
    percent = SIZE_MAX;
  }
  // Set major gc thresold to minimum major heap size
  mem->major_gc_threshold_words = mem->minor->size * MIN_MAJOR_HEAP_FACTOR;
  // To calculate percentage, split allocated into two part
  const size_t lo = mem->major_allocated_words % 100;
  const size_t hi = mem->major_allocated_words / 100;
  // Calculate percentage with checking overflow
  const size_t lo_percented = lo * percent / 100;
  size_t hi_percented = hi * percent;
  if(hi_percented / percent != hi) {
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
  if(percented > mem->major_gc_threshold_words) {
    mem->major_gc_threshold_words = percented;
  }
}

void initMem(Mem* mem,
  size_t minor_heap_words,
  size_t major_gc_threshold_percent,
  size_t stack_size,
  size_t global_size) {
  memset(mem, 0, sizeof(Mem));

  mem->minor = newStack(minor_heap_words, 1);

  mem->major_gc_threshold_percent = major_gc_threshold_percent;
  mem->stack = newStack(stack_size, 1);
  mem->global = newStack(global_size, 0);

  updateMajorGCThreshold(mem);
}

void finiMem(Mem* mem) {
  // Free minor heap
  if(mem->minor != NULL) {
    freeStack(mem->minor);
    mem->minor = NULL;
  }
  // Free major heap
  int m;
  ssmT i, t;
  for(m = 0; m < MAJOR_LIST_KINDS; m++) {
    for(i = mem->major_list[m]; i != NULL; i = t) {
      t = ssmTNext(i);
      free(i - SSM_MAJOR_TUP_EXTRA_WORDS);
    }
    mem->major_list[m] = NULL;
  }
  // Free stacks
  if(mem->stack != NULL) {
    freeStack(mem->stack);
    mem->stack = NULL;
  }
  if(mem->global != NULL) {
    freeStack(mem->global);
    mem->global = NULL;
  }
}

static inline ssmT allocMajorUninited(Mem* mem, ssmV words, int kind) {
  const size_t total = SSM_MAJOR_TUP_EXTRA_WORDS + words;
  void* p = aligned_alloc(SSM_WORD_SIZE, total * SSM_WORD_SIZE);
  if(p == NULL) {
    panicf("Failed to alloc major uninited of size %zu", words);
  }
  mem->major_allocated_words += total;
  ssmT tup = ((ssmT)p) + SSM_MAJOR_TUP_EXTRA_WORDS;
  ssmTNext(tup) = mem->major_list[kind];
  mem->major_list[kind] = tup;
  return tup;
}

static inline ssmT allocMajorLong(Mem* mem, ssmV bytes) {
  const size_t words = ssmTWordsFromBytes(bytes);
  ssmT tup = allocMajorUninited(mem, words, MAJOR_LIST_LEAVES);
  ssmTHd(tup) = ssmLongHd(bytes);
  return tup;
}

static inline ssmT allocMajorShort(Mem* mem, ssmV tag, ssmV words) {
  ssmT tup = allocMajorUninited(mem, ssmTWords(words), MAJOR_LIST_NODES);
  ssmTHd(tup) = ssmShortHd(tag, words);
  return tup;
}

static inline ssmT allocMinorUninited(Mem *mem, ssmV words) {
  mem->minor->top -= words + SSM_MINOR_TUP_EXTRA_WORDS;
  const ssmT top = ((ssmT) mem->minor->vals) + mem->minor->top;
  return top + SSM_MINOR_TUP_EXTRA_WORDS;
}

static inline ssmT allocMinorLongUnchecked(Mem* mem, ssmV bytes) {
  ssmT tup = allocMinorUninited(mem, ssmTWordsFromBytes(bytes));
  // Initialize tuple header
  ssmTHd(tup) = ssmLongHd(bytes);
  logf("(Alloc) long %p (bytes %zu)\n", tup, bytes);
  logf("     .. Stack %zu/%zu\n", mem->minor->top, mem->minor->size);
  return tup;
}

static inline ssmT allocMinorShortUnchecked(Mem* mem, ssmV tag, ssmV words) {
  ssmT tup = allocMinorUninited(mem, ssmTWords(words));
  // Initialize tuple header
  logf("Hd: %zx\n", ssmShortHd(tag, words));
  ssmTHd(tup) = ssmShortHd(tag, words);
  logf("(Alloc) short %p (words %zu)\n", tup, words);
  logf("     .. Stack %zu/%zu\n", mem->minor->top, mem->minor->size);
  return tup;
}

typedef int (*MarkableFn)(Mem*, ssmT);

static void markVal(Mem *mem, ssmV val, MarkableFn markable) {
  if(!ssmIsGCVal(val)) return;
  ssmT tup = ssmVal2Tup(val);
  if(!markable(mem, tup)) return;
  const ssmV hd = ssmTHd(tup);
  if(ssmHdColor(hd)) return;
  logf("(Mark) Mark stack -> %p\n", tup);
  ssmTHd(tup) = ssmHdMarked(hd);
  if(ssmHdIsLong(hd)) return;
  ssmTMarkList(tup) = mem->mark_list;
  mem->mark_list = tup;
  while(mem->mark_list != NULL) {
    ssmT marked_tup = mem->mark_list;
    mem->mark_list = ssmTMarkList(marked_tup);
    const ssmV words = ssmHdShortWords(ssmTHd(marked_tup));
    ssmV i;
    for(i = 0; i < words; i++) {
      if(!ssmIsGCVal(ssmTElem(marked_tup, i))) continue;
      ssmT tup = ssmVal2Tup(ssmTElem(marked_tup, i));
      if(!markable(mem, tup)) continue;
      const ssmV hd = ssmTHd(tup);
      if(ssmHdColor(hd)) continue;
      logf("(Mark) Mark %p -> %p\n", marked_tup, tup);
      ssmTHd(tup) = ssmHdMarked(hd);
      if(ssmHdIsLong(hd)) return;
      ssmTMarkList(tup) = mem->mark_list;
      mem->mark_list = tup;
    }
  }
}

static void markPhase(Mem* mem, MarkableFn markable) {
  ssmV i;
  // Mark global stack
  logf("(Mark) Marking global stack\n");
  for(i = 0; i < mem->global->top; i++) {
    markVal(mem, mem->global->vals[i], markable);
  }
  // Mark stack
  logf("(Mark) Marking call stack\n");
  for(i = mem->stack->top; i < mem->stack->size; i++) {
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
  ssmV m;
  for(m = MAJOR_LIST_LEAVES; m <= MAJOR_LIST_NODES; m++) {
    ssmT *lst = &mem->major_list[m];
    for(;;) {
      ssmT next = *lst;
      if(next == NULL) break;
      const ssmV hd = ssmTHd(next);
      if(!ssmHdColor(hd)) {
        // Free
        const ssmV words =
          ssmTWords(ssmHdWords(hd)) + SSM_MAJOR_TUP_EXTRA_WORDS;
        mem->major_allocated_words -= words;
        *lst = ssmTNext(next);
        free(next - SSM_MAJOR_TUP_EXTRA_WORDS);
      } else {
        // Unmark
        ssmTHd(next) = ssmHdUnmarked(ssmTHd(next));
        lst = &ssmTNext(next);
      }
    }
  }
}

static void moveMinorToMajor(Mem* mem) {
  // First, record last short list
  ssmT last_short = mem->major_list[MAJOR_LIST_NODES];
  // Then, copy all marked objects to major heap
  // and write new address into old tuple (header position)
  ssmT ptr = mem->minor->vals + mem->minor->top;
  ssmT lim = mem->minor->vals + mem->minor->size;
  while(ptr < lim) {
    const ssmV hd = ssmTHd(ptr);
    const ssmV words = ssmHdWords(hd);
    if(ssmHdColor(hd)) { // Marked Object
      // Allocate new major
      ssmT new_tup;
      if(ssmHdIsLong(hd)) {
        logf("(Move) alloc long (%zu)\n", ssmHdLongBytes(hd));
        new_tup = allocMajorLong(mem, ssmHdLongBytes(hd));
      } else {
        logf("(Move) alloc long (tag %zu size %zu)\n", ssmHdTag(hd), words);
        new_tup = allocMajorShort(mem, ssmHdTag(hd), words);
      }
      // Copy all elements
      memcpy(&ssmTElem(new_tup, 0), &ssmTElem(ptr, 0), SSM_WORD_SIZE * words);
      logf("(Move) %zx\n", ssmTHd(new_tup));
      // Write new address into old tuple
      ssmTHd(ptr) = ssmTup2Val(new_tup);
    }
    ptr += ssmTWords(words) + SSM_MINOR_TUP_EXTRA_WORDS;
  }
  // Traverse all short lists and readdressing
  ssmT tup = mem->major_list[MAJOR_LIST_NODES];
  for(; tup != NULL && tup != last_short; tup = ssmTNext(tup)) {
    const ssmV words = ssmHdShortWords(ssmTHd(tup));
    ssmV i;
    for(i = 0; i < words; i++) {
      ssmV v = ssmTElem(tup, i);
      if(!ssmIsGCVal(v)) continue;
      ssmT e_tup = ssmVal2Tup(v);
      if(inStack(mem->minor, e_tup)) {
        ssmTElem(tup, i) = ssmTHd(e_tup);
      }
    }
  }
  // Traverse all global & stacks and readdressing
  ssmV i;
  for(i = 0; i < mem->global->top; i++) {
    ssmV v = mem->global->vals[i];
    if(!ssmIsGCVal(v)) continue;
    ssmT e_tup = ssmVal2Tup(v);
    if(inStack(mem->minor, e_tup)) {
      mem->global->vals[i] = ssmTHd(e_tup);
    }
  }
  for(i = mem->stack->top; i < mem->stack->size; i++) {
    ssmV v = mem->stack->vals[i];
    if(!ssmIsGCVal(v)) continue;
    ssmT e_tup = ssmVal2Tup(v);
    if(inStack(mem->minor, e_tup)) {
      mem->stack->vals[i] = ssmTHd(e_tup);
    }
  }
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
  logf("(Minor %zd) Start\n", mem->minor_gc_count);
  // Check major heap is full
  size_t minor_allocated = mem->minor->size - mem->minor->top;
  size_t major_allocated_guess = mem->major_allocated_words;
  if(major_allocated_guess > SIZE_MAX - minor_allocated) {
    // Overflowed
    major_allocated_guess = SIZE_MAX;
  } else {
    major_allocated_guess += minor_allocated;
  }
  logf("(Minor %zd) guess = %zu\n", mem->minor_gc_count, major_allocated_guess);
  if(major_allocated_guess >= mem->major_gc_threshold_words) {
    // Major heap is full, do full GC
    logf("(Minor %zd) Run full GC\n", mem->minor_gc_count);
    return fullGC(mem);
  }
  // Run marking phase
  logf("(Minor %zd) Marking...\n", mem->minor_gc_count);
  markPhase(mem, markableMinor);
  // Move marked objects to major heap
  logf("(Minor %zd) Moving...\n", mem->minor_gc_count);
  moveMinorToMajor(mem);
  // Rewind minor heap's top pointer (left)
  logf("(Minor %zd) Rewinding...\n", mem->minor_gc_count);
  mem->minor->top = mem->minor->size;
  // Update statistics
  logf("(Minor %zd) Done\n", mem->minor_gc_count);
  mem->minor_gc_count++;
  return 0;
}

ssmT newLongTup(Mem *mem, ssmV bytes) {
  // Get size
  const size_t size = ssmTWordsFromBytes(bytes) + SSM_MINOR_TUP_EXTRA_WORDS;
  // Shortcut to allocate
  if(size <= mem->minor->top) {
    return allocMinorLongUnchecked(mem, bytes);
  } else if(mem->minor->size < size) {
    // Minor heap is not enough, just alloc in major
    return allocMajorLong(mem, bytes);
  } else {
    // Minor heap is enough, but not enough space
    // Do minor GC
    minorGC(mem);
    return allocMinorLongUnchecked(mem, bytes);
  }
}

ssmT newTup(Mem *mem, ssmV tag, ssmV words) {
  // Get size
  const size_t size = ssmTWords(words) + SSM_MINOR_TUP_EXTRA_WORDS;
  // Shortcut to allocate
  if(size <= mem->minor->top) {
    return allocMinorShortUnchecked(mem, tag, words);
  } else if(mem->minor->size < size) {
    // Minor heap is not enough
    // In this case, first run minor GC for invariant
    if(minorGC(mem) < 0) {
      panic("Minor GC failed");
    }
    return allocMajorShort(mem, tag, words);
  } else {
    minorGC(mem);
    return allocMinorShortUnchecked(mem, tag, words);
  }
}