/**
 * @file ssm_gc.c
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

/*
 * ssm GC is a mark-and-sweep GC with two generation.
 * The design is a kind of simplified version of the one in OCaml.
 * 
 * Steps of GC:
 * 1. Mark all reachable objects
 *   - Root objects are in stack, global and write_barrier list
 *   - In minor GC, only objects in minor heap can be marked
 *     - However, some major objects passed write barrier are also checked
 *   - In major GC, all objects can be marked
 * 2. (Major GC) Free unreachable major objeets
 * 3. Move reachable objects to new minor heap
 *   - First, copy minor objects in the major heap
 *   - Then write new major object pointer into minor object (in header)
 *   - Traverse all object may contain minor objects and update them
 * 4. Clean-up
 *   - Rewind minor heap stack pointer, clean write_barrier, etc.
 * 
 * Some notes:
 * - NULL pointer is not a target of GC. Initialize tuples with NULL for safety
 * - GC only runs when allocation occurred.
 * - Alloc does not give initialized tuple.
 *   You need to initialize it before GC runs.
 * - If you changed a value of some old tuple, you need to call writeBarrier.
 * - GC is not thread-safe. Use separate GC for each thread.
 */


#include <ssm_i.h>

// -- Initialization / Finalization

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

// -- Allocation helpers

static inline ssmT allocMajorUninited(Mem* mem, ssmV words, int kind) {
  // Allocate a tuple in major heap without initialization
  // kind is one of MAJOR_LIST_*
  // Also words must include header size (using ssmTWords*)
  const size_t total = SSM_MAJOR_TUP_EXTRA_WORDS + words;
  void* p = aligned_alloc(SSM_WORD_SIZE, total * SSM_WORD_SIZE);
  if(p == NULL) {
    panicf("Failed to alloc major uninited of size %zu", words);
  }
  // Increase allocated words
  mem->major_allocated_words += total;
  // Hide extra words
  ssmT tup = ((ssmT)p) + SSM_MAJOR_TUP_EXTRA_WORDS;
  // Insert to major list
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
  // ALlocate tuple in minor heap without initialization
  // Also words must include header size (using ssmTWords*)
  mem->minor->top -= words + SSM_MINOR_TUP_EXTRA_WORDS;
  const ssmT top = ((ssmT) mem->minor->vals) + mem->minor->top;
  return top + SSM_MINOR_TUP_EXTRA_WORDS;
}

static inline ssmT allocMinorLongUnchecked(Mem* mem, ssmV bytes) {
  // Note: it does not check if free space of minor heap is enough!
  ssmT tup = allocMinorUninited(mem, ssmTWordsFromBytes(bytes));
  // Initialize tuple header
  ssmTHd(tup) = ssmLongHd(bytes);
  gcLogf("(Alloc) long %p (bytes %zu) => Stack %zu/%zu\n", tup, bytes, mem->minor->top, mem->minor->size);
  return tup;
}

static inline ssmT allocMinorShortUnchecked(Mem* mem, ssmV tag, ssmV words) {
  // Note: it does not check if free space of minor heap is enough!
  ssmT tup = allocMinorUninited(mem, ssmTWords(words));
  // Initialize tuple header
  ssmTHd(tup) = ssmShortHd(tag, words);
  gcLogf("(Alloc) short %p (tag %zu, words %zu) => Stack %zu/%zu\n", tup, tag, words, mem->minor->top, mem->minor->size);
  return tup;
}

// -- GC Marking Phase

typedef int (*MarkableFn)(Mem*, ssmT);

static inline void markAndPush(Mem *mem, ssmV val, MarkableFn markable) {
  // Try to mark val.
  // If it succeed & may contains other tuples (i.e. short object),
  // push it into mark list.
  if(!ssmIsGCVal(val)) return;
  ssmT tup = ssmVal2Tup(val);
  if(!markable(mem, tup)) return;
  const ssmV hd = ssmTHd(tup);
  if(ssmHdColor(hd)) return;
  gcLogf("(Mark) -> %p\n", tup);
  ssmTHd(tup) = ssmHdMarked(hd);
  // Long tuple cannot contain other tuples. Thus ignore it
  if(ssmHdIsLong(hd)) return;
  ssmTMarkList(tup) = mem->mark_list;
  mem->mark_list = tup;
}

static inline void markElems(Mem *mem, ssmT marked_tup, MarkableFn markable) {
  // Mark all elements of marked_tup (it must be short tuple)
  // Note that if marked_tup is not marked, inf-loop may occur
  // Extract headers and number of elements
  const ssmV hd = ssmTHd(marked_tup);
  const ssmV words = ssmHdShortWords(hd);
  ssmV i;
  gcLogf("(Mark) Pick %p(%zu)[-/%zu]\n", marked_tup, ssmHdTag(hd), words);
  // Loop over all elements
  for(i = 0; i < words; i++) {
    const ssmV elem = ssmTElem(marked_tup, i);
    gcLogf("(Mark) Ref Exists %p(%zu)[%zu/%zu] -> 0x%zx\n", marked_tup, ssmHdTag(hd), i, words, elem);
    markAndPush(mem, elem, markable);
  }
}

static void markPhase(Mem* mem, MarkableFn markable) {
  ssmV i;
  // Traverse write barrier and mark all elements
  gcLogf("(Mark) --- write barrier ---\n");
  ssmT *lst = &mem->write_barrier;
  for(;;) {
    const ssmT tup = *lst;
    if(tup == NULL) break;
    else if(ssmHdIsLong(ssmTHd(tup)) || inStack(mem->minor, tup)) {
      // Long tuple are rejected because it cannot contain other tuples
      // Tuple in minor heap are rejected because write barrier list may
      // corrupted by copying minor heap to major heap, and they are markable
      // for any GC.
      gcLogf("(Mark) Reject %p\n", tup);
      *lst = ssmTMarkList(tup);
      ssmTHd(tup) = ssmHdUnmarked(ssmTHd(tup));
    } else {
      lst = &ssmTMarkList(tup);
      markElems(mem, tup, markable);
    }
  }
  // Mark global stack
  gcLogf("(Mark) --- global stack ---\n");
  for(i = 0; i < mem->global->top; i++) {
    markAndPush(mem, mem->global->vals[i], markable);
  }
  // Mark stack
  gcLogf("(Mark) --- call stack ---\n");
  for(i = mem->stack->top; i < mem->stack->size; i++) {
    markAndPush(mem, mem->stack->vals[i], markable);
  }
  // Mark elemes in mark_list and remove them
  while(mem->mark_list != NULL) {
    ssmT marked_tup = mem->mark_list;
    mem->mark_list = ssmTMarkList(marked_tup);
    markElems(mem, marked_tup, markable);
  }
}

static int markableMajor(Mem* mem, ssmT tup) {
  // During major GC, all tuples (except NULL pointer) are markable
  (void) mem;
  return tup != NULL;
}

static int markableMinor(Mem* mem, ssmT tup) {
  // During minor GC, only tuples in minor heap are markable
  return inStack(mem->minor, tup);
}

// -- GC Free Unmarked Major Phase

static void freeUnmarkedMajor(Mem* mem) {
  // Free all unmarked (= unreachable) tuples in major heap
  // If some tuples are marked (= reachable), unmark them for next GC.
  ssmV m;
  for(m = MAJOR_LIST_LEAVES; m <= MAJOR_LIST_NODES; m++) {
    ssmT *lst = &mem->major_list[m];
    for(;;) {
      ssmT next = *lst;
      if(next == NULL) break;
      const ssmV hd = ssmTHd(next);
      if(ssmHdColor(hd) != SSM_COLOR_BLACK) {
        // Calculate Size
        const ssmV words =
          ssmTWords(ssmHdWords(hd)) + SSM_MAJOR_TUP_EXTRA_WORDS;
        mem->major_allocated_words -= words;
        // Remove from list
        *lst = ssmTNext(next);
        // Free tuple
        gcLogf("(Free) %p (words %zu)\n", next, words);
        free(next - SSM_MAJOR_TUP_EXTRA_WORDS);
      } else {
        // Unmark
        ssmTHd(next) = ssmHdUnmarked(ssmTHd(next));
        // Iterate to the next tuple
        lst = &ssmTNext(next);
      }
    }
  }
}

// -- GC Move Minor to Major Phase

static inline void readdressMovedTuple(Mem *mem, ssmV *val) {
  const ssmV v = *val;
  if(!ssmIsGCVal(v)) return;
  ssmT e_tup = ssmVal2Tup(v);
  gcLogf("(Move) Ref *%p = %p\n", val, e_tup);
  if(!inStack(mem->minor, e_tup)) return;
  *val = ssmTHd(e_tup);
  gcLogf("       -> %p\n", ssmVal2Tup(*val));
}

static void moveMinorToMajor(Mem* mem) {
  // First, record last short list
  // It is the first element of old major tuples
  // = the next element of new major tuples
  ssmT last_short = mem->major_list[MAJOR_LIST_NODES];
  // Then, copy all marked objects to major heap
  // and write new address into old tuple (header position)
  gcLogf("(Move) --- minor to major ---\n");
  ssmT ptr = mem->minor->vals + mem->minor->top + SSM_MINOR_TUP_EXTRA_WORDS;
  ssmT lim = mem->minor->vals + mem->minor->size;
  while(ptr < lim) {
    const ssmV hd = ssmTHd(ptr);
    const ssmV words = ssmHdWords(hd);
    gcLogf("(Move) ptr = %p, hd = 0x%zx\n", ptr, ssmTHd(ptr));
    gcLogf("       words = %zu\n", words);
    if(ssmHdColor(hd)) { // Marked Object
      // Allocate new major
      ssmT new_tup;
      if(ssmHdIsLong(hd)) {
        gcLogf("       alloc long (%zu)\n", ssmHdLongBytes(hd));
        new_tup = allocMajorLong(mem, ssmHdLongBytes(hd));
      } else {
        gcLogf("       alloc short (tag %zu size %zu)\n", ssmHdTag(hd), words);
        new_tup = allocMajorShort(mem, ssmHdTag(hd), words);
      }
      gcLogf("       %p -> %p\n", ptr, new_tup);
      gcLogf("       %zx %zx\n", ptr[1], ptr[2]);
      // Copy all elements
      memcpy(&ssmTElem(new_tup, 0), &ssmTElem(ptr, 0), SSM_WORD_SIZE * words);
      gcLogf("(Move) hd = %zx\n", ssmTHd(new_tup));
      // Write new address into old tuple
      ssmTHd(ptr) = ssmTup2Val(new_tup);
    }
    ptr += ssmTWords(words) + SSM_MINOR_TUP_EXTRA_WORDS;
  }
  // Traverse all short lists and readdressing
  gcLogf("(Move) --- traverse short ---\n");
  ssmT tup = mem->major_list[MAJOR_LIST_NODES];
  for(; tup != NULL && tup != last_short; tup = ssmTNext(tup)) {
    const ssmV words = ssmHdShortWords(ssmTHd(tup));
    gcLogf("(Move) Moved minor tup %p (%zu)\n", tup, words);
    ssmV i;
    for(i = 0; i < words; i++) {
      readdressMovedTuple(mem, &ssmTElem(tup, i));
    }
  }
  // Traverse all write barrier and readdressing
  gcLogf("(Move) --- traverse write barrier ---\n");
  for(tup = mem->write_barrier; tup != NULL; tup = ssmTMarkList(tup)) {
    ssmV hd = ssmTHd(tup);
    // Unmark
    ssmTHd(tup) = ssmHdUnmarked(hd);
    // Readdressing
    const ssmV words = ssmHdShortWords(hd);
    gcLogf("(Move) Write barrier %p (%zu)\n", tup, words);
    ssmV i;
    for(i = 0; i < words; i++) {
      readdressMovedTuple(mem, &ssmTElem(tup, i));
    }
  }
  // Traverse all global & stacks and readdressing
  gcLogf("(Move) --- traverse global ---\n");
  ssmV i;
  for(i = 0; i < mem->global->top; i++) {
    readdressMovedTuple(mem, &mem->global->vals[i]);
  }
  gcLogf("(Move) --- traverse stack ---\n");
  for(i = mem->stack->top; i < mem->stack->size; i++) {
    readdressMovedTuple(mem, &mem->stack->vals[i]);
  }
}

// -- GC APIs

int fullGC(Mem* mem) {
  gcLogf("(Full %zd) Start\n", mem->major_gc_count);
  // Run marking phase
  gcLogf("(Full %zd) Marking...\n", mem->major_gc_count);
  markPhase(mem, markableMajor);
  // Traverse object list and free unmarked objects
  // also unmark all marked objects
  gcLogf("(Full %zd) Freeing unmarked...\n", mem->major_gc_count);
  freeUnmarkedMajor(mem);
  // Move marked object in minor heap to major heap
  gcLogf("(Full %zd) Move minor to major...\n", mem->major_gc_count);
  moveMinorToMajor(mem);
  // Rewind minor heap's top pointer (left)
  gcLogf("(Full %zd) Rewinding minor...\n", mem->major_gc_count);
  mem->minor->top = mem->minor->size;
  // Adjust major gc threshold
  gcLogf("(Full %zd) Updating major GC threshold...\n", mem->major_gc_count);
  updateMajorGCThreshold(mem);
  // Clean write barrier
  mem->write_barrier = NULL;
  // Update statistics
  gcLogf("(Full %zd) Done\n", mem->major_gc_count);
  mem->major_gc_count++;
  return 0;
}

int minorGC(Mem* mem) {
  gcLogf("(Minor %zd) Start\n", mem->minor_gc_count);
  // Check major heap is full
  size_t minor_allocated = mem->minor->size - mem->minor->top;
  size_t major_allocated_guess = mem->major_allocated_words;
  if(major_allocated_guess > SIZE_MAX - minor_allocated) {
    // Overflowed
    major_allocated_guess = SIZE_MAX;
  } else {
    major_allocated_guess += minor_allocated;
  }
  gcLogf("(Minor %zd) guess = %zu, thre = %zu\n",
    mem->minor_gc_count, major_allocated_guess, mem->major_gc_threshold_words);
  if(major_allocated_guess >= mem->major_gc_threshold_words) {
    // Major heap is full, do full GC
    gcLogf("(Minor %zd) Run full GC\n", mem->minor_gc_count);
    return fullGC(mem);
  }
  // Run marking phase
  gcLogf("(Minor %zd) Marking...\n", mem->minor_gc_count);
  markPhase(mem, markableMinor);
  // Move marked objects to major heap
  gcLogf("(Minor %zd) Moving...\n", mem->minor_gc_count);
  moveMinorToMajor(mem);
  // Rewind minor heap's top pointer (left)
  gcLogf("(Minor %zd) Rewinding...\n", mem->minor_gc_count);
  mem->minor->top = mem->minor->size;
  // Clean write barrier
  mem->write_barrier = NULL;
  // Update statistics
  gcLogf("(Minor %zd) Done\n", mem->minor_gc_count);
  mem->minor_gc_count++;
  return 0;
}

// -- Allocations APIs

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

// -- Write Barriers

void gcWriteBarrier(Mem *mem, ssmT tup) {
  const ssmV hd = ssmTHd(tup);
  if(ssmHdColor(hd)) return;
  gcLogf("Write barrier: %p\n", tup);
  // If the object is white, mark it gray
  ssmTHd(tup) = ssmHdMarked(hd);
  // Push to write_barrier list
  ssmTMarkList(tup) = mem->write_barrier;
  mem->write_barrier = tup;
}

// -- Checking helpers

static void checkMemTupInvariants(ssmT tup) {
  const ssmV hd = ssmTHd(tup);
  if(!ssmHdIsLong(hd)) {
    const ssmV words = ssmHdShortWords(hd);
    ssmV i;
    for(i = 0; i < words; i++) {
      ssmV e = ssmTElem(tup, i);
      if(e > 0x200000000ULL) {
        panicf("Invalid value in tup %p[%zu/%zu]: %zu = 0x%zx\n", tup, i, words, e, e);
      }
    }
  }
}

void checkMemInvariants(Mem *mem) {
  // Test functions
  { // Check traverse minor heap
    ssmT ptr = mem->minor->vals + mem->minor->top;
    ssmT lim = mem->minor->vals + mem->minor->size;
    while(ptr < lim) {
      const ssmT tup = ptr + SSM_MINOR_TUP_EXTRA_WORDS;
      const ssmV hd = ssmTHd(tup);
      const ssmV words = ssmHdWords(hd);
      const ssmV twords = ssmTWords(words);
      checkMemTupInvariants(tup);
      ptr += twords + SSM_MINOR_TUP_EXTRA_WORDS;
    }
    if(ptr != lim) {
      panic("Minor heap headers may be corrupted");
    }
  }
  { // Check traverse major heap
    ssmV m;
    for(m = 0; m < MAJOR_LIST_KINDS; m++) {
      ssmT lst = mem->major_list[m];
      for(; lst != NULL; lst = ssmTNext(lst)) {
        checkMemTupInvariants(lst);
      }
    }
  }
  { // Check traverse stack
    ssmV i;
    for(i = mem->stack->top; i < mem->stack->size; i++) {
      ssmV e = mem->stack->vals[i];
      if(e > 0x200000000ULL) {
        panicf("Invalid value in stack[%zu/%zu]: %zu = 0x%zx\n", i, mem->stack->size, e, e);
      }
    }
  }
}