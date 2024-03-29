#include <ssm.h>
#include <ssm_i.h>

#define ASSERT(cond) \
  if(!(cond)) { \
    fprintf(stderr, "[FATAL] (ssm) Assert failed at %s:%d: %s\n", \
      __FILE__, __LINE__, #cond); \
    return -1; \
  }
#define ASSERT_EQ(a, b) \
  if((a) != (b)) { \
    fprintf(stderr, "[FATAL] (ssm) Assert failed at %s:%d: %s != %s\n", \
      __FILE__, __LINE__, #a, #b); \
    return -1; \
  }

int test_cnt = 0, pass_cnt = 0;
#define TEST(name) { \
  test_cnt++; \
  printf("[INFO] Test %d (%s) start...\n", test_cnt, #name); \
  if(name() < 0) { \
    printf("[FAIL] Test %d (%s) failed!\n", test_cnt, #name); \
  } else {\
    pass_cnt++; \
    printf("[PASS] Test %d (%s) passed\n", test_cnt, #name); \
  } \
}

// ---------------------------------------------------------

int alwaysPass() {
  ASSERT(1);
  ASSERT_EQ(42, 1 + 41);
  return 0;
}

// ---------------------------------------------------------
// Value test

int testValue0() {
  ssmV value;
  // int
  value = ssmInt2Val(42);
  ASSERT_EQ(ssmVal2Int(value), 42);
  ASSERT(ssmIsLiteral(value));
  ASSERT(!ssmIsGCVal(value));
  // neg int
  value = ssmInt2Val(-42);
  ASSERT_EQ(ssmVal2Int(value), -42);
  ASSERT(ssmIsLiteral(value));
  ASSERT(!ssmIsGCVal(value));
  // uint
  value = ssmUint2Val(42);
  ASSERT_EQ(ssmVal2Uint(value), 42);
  ASSERT(ssmIsLiteral(value));
  ASSERT(!ssmIsGCVal(value));
  // float
  value = ssmFlt2Val(0.75);
  ASSERT_EQ(ssmVal2Flt(value), 0.75);
  ASSERT(ssmIsLiteral(value));
  ASSERT(!ssmIsGCVal(value));
  // non-gc pointer
  value = ssmPtr2Val(&value);
  ASSERT_EQ(ssmVal2Ptr(void, value), (void*)&value);
  ASSERT(ssmIsLiteral(value));
  ASSERT(!ssmIsGCVal(value));
  // gc pointer
  ssmT tup = (ssmT) malloc(31);
  value = ssmTup2Val(tup);
  ASSERT_EQ(ssmVal2Tup(value), tup);
  ASSERT(!ssmIsLiteral(value));
  ASSERT(ssmIsGCVal(value));
  return 0;
}

void valueTest() {
  TEST(testValue0);
}

// ---------------------------------------------------------
// GC Test


// -- Checking helpers

static void checkMemTupInvariants(ssmT tup) {
  const ssmV hd = ssmTHd(tup);
  if(!ssmHdIsLong(hd)) {
    const ssmV words = ssmHdShortWords(hd);
    ssmV i;
    for(i = 0; i < words; i++) {
      ssmV e = ssmTElem(tup, i);
      if(e > 0x200000000ULL) {
        panicf("Invalid value in tup %p[%zu/%zu]: %zu = 0x%zx\n",
          tup, i, words, e, e);
      }
    }
  }
}

void ssmCheckMemInvariants(ssmMem *mem) {
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
    for(m = 0; m < SSM_MAJOR_LIST_KINDS; m++) {
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

int testGC0() {
  ssmMem mem;
  ssmInitMem(&mem, 1024, 50, 1024, 1024);
  ssmNewTup(&mem, 1, 42);
  ssmFiniMem(&mem);
  return 0;
}

int testGC1() {
  // Check minor allocation gap
  size_t word_size = sizeof(void*);
  ASSERT_EQ(word_size, SSM_WORD_SIZE);
  ssmMem mem;
  ssmInitMem(&mem, 1024, 50, 1024, 1024);
  ssmT v1 = ssmNewTup(&mem, 1, 8);
  ssmT v2 = ssmNewTup(&mem, 1, 4);
  ssmT v3 = ssmNewTup(&mem, 1, 7);
  ssmT v4 = ssmNewLongTup(&mem, 2 + word_size * 3);
  ssmT v5 = ssmNewTup(&mem, 1, 1);
  ASSERT_EQ(v5 + 3, v4);
  ASSERT_EQ(v4 + 6, v3);
  ASSERT_EQ(v3 + 9, v2);
  ASSERT_EQ(v2 + 6, v1);
  ASSERT_EQ(v1, ((ssmT)mem.minor->vals) + (mem.minor->size - 9));
  ssmFiniMem(&mem);
  return 0;
}

int testGC2() {
  // Run minor GC many times
  ssmMem mem;
  ssmInitMem(&mem, 32, 50, 1024, 1024);
  int i;
  for(i = 0; i < 10000; i++) {
    ssmNewTup(&mem, 1, 20);
  }
  ssmFiniMem(&mem);
  return 0;
}

int testGC3() {
  // Run GC a few times
  ssmMem mem;
  ssmInitMem(&mem, 100, 50, 1024, 1024);
  // Create tuples
  ssmNewTup(&mem, 1, 5);
  ssmT v2 = ssmNewTup(&mem, 2, 5);
  ssmNewTup(&mem, 3, 5);
  ssmT v4 = ssmNewTup(&mem, 4, 5);
  ssmNewTup(&mem, 5, 5);
  // Only save v2 and v4
  ssmPushStackR(mem.stack, ssmTup2Val(v2));
  ssmPushStackR(mem.stack, ssmTup2Val(v4));
  logf("%zx\n", ssmTHd(v2));
  ASSERT_EQ(ssmHdTag(ssmTHd(v2)), 2);
  ASSERT_EQ(ssmHdTag(ssmTHd(v4)), 4);
  // Run minor GC 2 times and major GC 1 time
  int i;
  for(i = 0; i < 3; i++) {
    // Clean up other tuples
    if(i == 2) ssmFullGC(&mem);
    else ssmMinorGC(&mem);
    // Then v2 and v4 should be moved into major heap
    ssmT nv2 = ssmVal2Tup(mem.stack->vals[mem.stack->size - 1]);
    ssmT nv4 = ssmVal2Tup(mem.stack->vals[mem.stack->size - 2]);
    // Check the fact.
    ASSERT(nv2 != v2);
    ASSERT(nv4 != v4);
    logf("%zx\n", ssmTHd(nv2));
    logf("%zx\n", ssmTHd(nv4));
    ASSERT_EQ(ssmHdTag(ssmTHd(nv2)), 2);
    ASSERT_EQ(ssmHdSize(ssmTHd(nv2)), 5);
    ASSERT_EQ(ssmHdTag(ssmTHd(nv4)), 4);
    ASSERT_EQ(ssmHdSize(ssmTHd(nv4)), 5);
    ASSERT_EQ(mem.major_allocated_words, 2 * (SSM_MAJOR_TUP_EXTRA_WORDS + 6));
  }
  // Clean-up
  ssmPopStackR(mem.stack);
  ssmPopStackR(mem.stack);
  ssmFullGC(&mem);
  ASSERT_EQ(mem.major_allocated_words, 0);
  ssmFiniMem(&mem);
  return 0;
}

int testGCRandom1() {
  int i;
  // Init
  ssmMem mem;
  ssmInitMem(&mem, 100, 50, 128, 128);
  const int stack_n = 4;
  // Push some NULL in stack
  for(i = 0; i < stack_n; i++) {
    ssmPushStackR(mem.stack, ssmTup2Val(NULL));
  }
  // Create many trees
  const int n = 1000000;
  
  for(i = 0; i < n; i++) {
    int r = rand() % 8;
    //printf("-- %6d: op %d\n", i, r);
    // First take a random number and do random operation
    switch(r) {
    case 0: case 4: case 6: {
      // Create a tuple and push into stack
      const size_t size = 2 + (rand() % 10);
      ssmT v = ssmNewTup(&mem, i, size);
      memset(&ssmTElem(v, 0), 0x00, size * SSM_WORD_SIZE);
      // Push it into stack
      const size_t idx = mem.stack->size - 1 - (rand() % stack_n);
      ssmTElem(v, rand() % size) = mem.stack->vals[idx];
      mem.stack->vals[idx] = ssmTup2Val(v);
    } break;
    case 1: case 5: case 7: {
      // Put a value in random position
      const size_t idx = mem.stack->size - 1 - (rand() % stack_n);
      const size_t size = 2 + (rand() % 10);
      ssmT v = ssmNewTup(&mem, i, size);
      memset(&ssmTElem(v, 0), 0x00, size * SSM_WORD_SIZE);
      if(ssmVal2Tup(mem.stack->vals[idx]) != NULL) {
        ssmT tup = ssmVal2Tup(mem.stack->vals[idx]);
        ssmTElem(tup, 0) = ssmTup2Val(v);
        ssmGCWriteBarrier(&mem, tup);
      }
    } break;
    case 2: {
      // Remove random index
      const size_t idx = mem.stack->size - 1 - (rand() % stack_n);
      mem.stack->vals[idx] = ssmTup2Val(NULL);
    } break;
    case 3: {
      // Just alloc
      ssmNewLongTup(&mem, 10 + rand() % 100);
    } break;
    }
    ssmCheckMemInvariants(&mem);
  }
  logf("Minor top/size: %zu/%zu words\n", mem.minor->top, mem.minor->size);
  logf("Major Allocated: %zu words\n", mem.major_allocated_words);
  // Fin
  ssmFiniMem(&mem);
  return 0;
}

int testGCRandom2() {
  int i;
  // Init
  ssmMem mem;
  ssmInitMem(&mem, 16384, 75, 128, 128);
  const int stack_n = 4;
  // Push some NULL in stack
  for(i = 0; i < stack_n; i++) {
    ssmPushStackR(mem.stack, ssmTup2Val(NULL));
  }
  // Create many trees
  const int n = 1000000;
  
  for(i = 0; i < n; i++) {
    if(i % 10000 == 0) {
      printf("%5d0k: (min %5zu maj %3zu) (maj allocated %10zu / %10zu)\n", i / 1000, mem.minor_gc_count, mem.major_gc_count, mem.major_allocated_words, mem.major_gc_threshold_words);
    }
    int r = rand() % 32;
    if(r > 7) r = 0;
    //printf("-- %6d: op %d\n", i, r);
    // First take a random number and do random operation
    switch(r) {
    case 0: case 4: case 6: {
      // Create a tuple and push into stack
      const size_t size = 2 + (rand() % 10);
      ssmT v = ssmNewTup(&mem, i, size);
      memset(&ssmTElem(v, 0), 0x00, size * SSM_WORD_SIZE);
      // Push it into stack
      const size_t idx = mem.stack->size - 1 - (rand() % stack_n);
      ssmTElem(v, rand() % size) = mem.stack->vals[idx];
      mem.stack->vals[idx] = ssmTup2Val(v);
    } break;
    case 1: case 5: case 7: {
      // Put a value in random position
      /*const size_t idx = mem.stack->size - 1 - (rand() % stack_n);
      const size_t size = 2 + (rand() % 10);
      ssmT v = ssmNewTup(&mem, i, size);
      memset(&ssmTElem(v, 0), 0x00, size * SSM_WORD_SIZE);
      if(ssmVal2Tup(mem.stack->vals[idx]) != NULL) {
        ssmT tup = ssmVal2Tup(mem.stack->vals[idx]);
        ssmTElem(tup, 0) = ssmTup2Val(v);
        ssmGCWriteBarrier(&mem, tup);
      }*/
    } break;
    case 2: {
      // Remove random index
      const size_t idx = mem.stack->size - 1 - (rand() % stack_n);
      mem.stack->vals[idx] = ssmTup2Val(NULL);
    } break;
    case 3: {
      // Just alloc
      ssmNewLongTup(&mem, 10 + rand() % 100);
    } break;
    }
  }
  logf("Minor top/size: %zu/%zu words\n", mem.minor->top, mem.minor->size);
  logf("Major Allocated: %zu words\n", mem.major_allocated_words);
  // Fin
  ssmFiniMem(&mem);
  return 0;
}

int testGCSize() {
  ASSERT_EQ(SSM_WORD_SIZE, sizeof(void*));
  ssmMem mem;
  ssmInitMem(&mem, 10, 50, 1024, 1024);
  ASSERT_EQ(mem.major_gc_threshold_percent, 50);
  ASSERT_EQ(mem.major_gc_threshold_words, 10 * GC_MIN_MAJOR_HEAP_FACTOR);
  // Check for multiple cases
  const size_t as[] = {121, 2521, 7721, 20000, 30000, 71201, 500000, 1775126};
  const size_t ps[] = {20, 50, 77, 100, 225, 333, 1000};
  // Change allocated size
  size_t i, j;
  for(i = 0; i < sizeof(as) / sizeof(size_t); i++) {
    mem.major_allocated_words = as[i];
    // Change threshold percent
    for(j = 0; j < sizeof(ps) / sizeof(size_t); j++) {
      mem.major_gc_threshold_percent = ps[j];
      ssmUpdateMajorGCThreshold(&mem);
      ASSERT_EQ(mem.major_gc_threshold_words, as[i] * (100 + ps[j]) / 100);
    }
  }
  // Check overflow 1
  mem.major_allocated_words = 10 + SIZE_MAX / 2;
  mem.major_gc_threshold_percent = 100;
  ssmUpdateMajorGCThreshold(&mem);
  ASSERT_EQ(mem.major_gc_threshold_words, SIZE_MAX);
  // Check overflow 2
  mem.major_allocated_words = 100;
  mem.major_gc_threshold_percent = SIZE_MAX;
  ssmUpdateMajorGCThreshold(&mem);
  ASSERT_EQ(mem.major_gc_threshold_words, SIZE_MAX);
  // Non-overflow
  mem.major_allocated_words = 1;
  mem.major_gc_threshold_percent = SIZE_MAX - 100;
  ssmUpdateMajorGCThreshold(&mem);
  ASSERT_EQ(mem.major_gc_threshold_words, SIZE_MAX / 100);
  ssmFiniMem(&mem);
  return 0;
}


void gcTest() {
  TEST(testGC0);
  TEST(testGC1);
  TEST(testGC2);
  TEST(testGC3);
  TEST(testGCRandom1);
  TEST(testGCRandom2);
  TEST(testGCSize);
}

// ---------------------------------------------------------


int main() {
  // Always pass
  TEST(alwaysPass);
  valueTest();
  gcTest();
  printf("[INFO] Test result: %d/%d passed\n", pass_cnt, test_cnt);
  return 0;
}