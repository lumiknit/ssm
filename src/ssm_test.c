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
  return 0;
}

int testGC2() {
  // Run minor GC many times
  Mem mem;
  initMem(&mem, SSM_WORD_SIZE * 32, 50, 1024, 1024);
  int i;
  for(i = 0; i < 10000; i++) {
    newTup(&mem, 1, 20);
  }
  return 0;
}

int testGC3() {
  // Run Minor GC
  Mem mem;
  initMem(&mem, SSM_WORD_SIZE * 32, 50, 1024, 1024);
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
  // Clean up other tuples
  minorGC(&mem);
  // Then v2 and v4 should be moved into major heap
  ssmT nv2 = ssmVal2Tup(mem.stack->vals[mem.stack->size - 1]);
  ssmT nv4 = ssmVal2Tup(mem.stack->vals[mem.stack->size - 2]);
  // Check the fact.
  ASSERT(nv2 != v2);
  ASSERT(nv4 != v4);
  logf("%zx\n", ssmTHd(nv2));
  logf("%zx\n", ssmTHd(nv4));
  ASSERT_EQ(ssmHdTag(ssmTHd(nv2)), 2);
  ASSERT_EQ(ssmHdTag(ssmTHd(nv4)), 4);
  ASSERT_EQ(mem.major_allocated_words, 2 * (SSM_MAJOR_TUP_EXTRA_WORDS + 6));
  return 0;
}

void gcTest() {
  TEST(testGC0);
  TEST(testGC1);
  TEST(testGC2);
  TEST(testGC3);
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