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

int testGC0() {
  Mem mem;
  initMem(&mem, 1024, 50, 1024, 1024);
  newTup(&mem, 1, 42);
  finiMem(&mem);
  return 0;
}

int testGC1() {
  // Check minor allocation gap
  size_t word_size = sizeof(void*);
  ASSERT_EQ(word_size, SSM_WORD_SIZE);
  Mem mem;
  initMem(&mem, 1024, 50, 1024, 1024);
  ssmT v1 = newTup(&mem, 1, 8);
  ssmT v2 = newTup(&mem, 1, 4);
  ssmT v3 = newTup(&mem, 1, 7);
  ssmT v4 = newLongTup(&mem, 2 + word_size * 3);
  ssmT v5 = newTup(&mem, 1, 1);
  ASSERT_EQ(v5 + 3, v4);
  ASSERT_EQ(v4 + 6, v3);
  ASSERT_EQ(v3 + 9, v2);
  ASSERT_EQ(v2 + 6, v1);
  ASSERT_EQ(v1, ((ssmT)mem.minor->vals) + (mem.minor->size - 9));
  finiMem(&mem);
  return 0;
}

int testGC2() {
  // Run minor GC many times
  Mem mem;
  initMem(&mem, 32, 50, 1024, 1024);
  int i;
  for(i = 0; i < 10000; i++) {
    newTup(&mem, 1, 20);
  }
  finiMem(&mem);
  return 0;
}

int testGC3() {
  // Run GC a few times
  Mem mem;
  initMem(&mem, 100, 50, 1024, 1024);
  // Create tuples
  newTup(&mem, 1, 5);
  ssmT v2 = newTup(&mem, 2, 5);
  newTup(&mem, 3, 5);
  ssmT v4 = newTup(&mem, 4, 5);
  newTup(&mem, 5, 5);
  // Only save v2 and v4
  pushStackR(mem.stack, ssmTup2Val(v2));
  pushStackR(mem.stack, ssmTup2Val(v4));
  logf("%zx\n", ssmTHd(v2));
  ASSERT_EQ(ssmHdTag(ssmTHd(v2)), 2);
  ASSERT_EQ(ssmHdTag(ssmTHd(v4)), 4);
  // Run minor GC 2 times and major GC 1 time
  int i;
  for(i = 0; i < 3; i++) {
    // Clean up other tuples
    if(i == 2) fullGC(&mem);
    else minorGC(&mem);
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
  popStackR(mem.stack);
  popStackR(mem.stack);
  fullGC(&mem);
  ASSERT_EQ(mem.major_allocated_words, 0);
  finiMem(&mem);
  return 0;
}

int testGCRandom1() {
  int i;
  // Init
  Mem mem;
  initMem(&mem, 100, 50, 128, 128);
  const int stack_n = 4;
  // Push some NULL in stack
  for(i = 0; i < stack_n; i++) {
    pushStackR(mem.stack, ssmTup2Val(NULL));
  }
  // Create many trees
  const int n = 10000;
  
  for(i = 0; i < n; i++) {
    int r = rand() % 4;
    printf("-- %6d: op %d\n", i, r);
    // First take a random number and do random operation
    switch(r) {
    case 0: {
      // Create a tuple and push into stack
      const size_t size = 2 + (rand() % 10);
      ssmT v = newTup(&mem, 1, size);
      memset(&ssmTElem(v, 0), 0x00, size * SSM_WORD_SIZE);
      // Push it into stack
      const size_t idx = mem.stack->size - 1 - (rand() % stack_n);
      ssmTElem(v, rand() % size) = mem.stack->vals[idx];
      mem.stack->vals[idx] = ssmTup2Val(v);
    } break;
    case 1: {
      // Put a value in random position
      const size_t idx = mem.stack->size - 1 - (rand() % stack_n);
      const size_t size = 2 + (rand() % 10);
      ssmT v = newTup(&mem, 4, size);
      memset(&ssmTElem(v, 0), 0x00, size * SSM_WORD_SIZE);
      if(ssmVal2Tup(mem.stack->vals[idx]) != NULL) {
        ssmT tup = ssmVal2Tup(mem.stack->vals[idx]);
        ssmTElem(tup, 0) = ssmTup2Val(v);
      }
    } break;
    case 2: {
      // Remove random index
      const size_t idx = mem.stack->size - 1 - (rand() % stack_n);
      mem.stack->vals[idx] = ssmTup2Val(NULL);
    } break;
    case 3: {
      // Just alloc
      newLongTup(&mem, 10 + rand() % 100);
    } break;
    }
  }
  // Fin
  finiMem(&mem);
  return 0;
}

int testGCSize() {
  ASSERT_EQ(SSM_WORD_SIZE, sizeof(void*));
  Mem mem;
  initMem(&mem, 10, 50, 1024, 1024);
  ASSERT_EQ(mem.major_gc_threshold_percent, 50);
  ASSERT_EQ(mem.major_gc_threshold_words, 10 * MIN_MAJOR_HEAP_FACTOR);
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
      updateMajorGCThreshold(&mem);
      ASSERT_EQ(mem.major_gc_threshold_words, as[i] * (100 + ps[j]) / 100);
    }
  }
  // Check overflow 1
  mem.major_allocated_words = 10 + SIZE_MAX / 2;
  mem.major_gc_threshold_percent = 100;
  updateMajorGCThreshold(&mem);
  ASSERT_EQ(mem.major_gc_threshold_words, SIZE_MAX);
  // Check overflow 2
  mem.major_allocated_words = 100;
  mem.major_gc_threshold_percent = SIZE_MAX;
  updateMajorGCThreshold(&mem);
  ASSERT_EQ(mem.major_gc_threshold_words, SIZE_MAX);
  // Non-overflow
  mem.major_allocated_words = 1;
  mem.major_gc_threshold_percent = SIZE_MAX - 100;
  updateMajorGCThreshold(&mem);
  ASSERT_EQ(mem.major_gc_threshold_words, SIZE_MAX / 100);
  finiMem(&mem);
  return 0;
}


void gcTest() {
  testGCRandom1();
  return;

  TEST(testGC0);
  TEST(testGC1);
  TEST(testGC2);
  TEST(testGC3);
  TEST(testGCRandom1);
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