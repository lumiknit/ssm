#include <ssm.h>

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

// ---------------------------------------------------------

int always_pass() {
  ASSERT(1);
  ASSERT_EQ(42, 1 + 41);
  return 0;
}

// ---------------------------------------------------------

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

int main() {
  TEST(always_pass);
  printf("[INFO] Test result: %d/%d passed\n", pass_cnt, test_cnt);
  return 0;
}