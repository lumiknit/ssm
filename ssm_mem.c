#include "ssm_mem.h"

#include <stdint.h>

void* a_alloc(size_t size) {
  uintptr_t ptr = (uintptr_t)malloc(size + SZ_VAL);
  if((void*)ptr == NULL) return NULL;
  uintptr_t aligned = (ptr + SZ_VAL) & ~(SZ_VAL - 1);
  ((uint8_t*)aligned)[-1] = (uint8_t)(aligned - ptr);
  return (void*)aligned;
}

void a_free(void *ptr) {
  uintptr_t offset = (uintptr_t)((uint8_t*)ptr)[-1];
  uintptr_t original = ((uintptr_t) ptr) - offset;
  free((void*)original);
}

void* a_realloc(void *ptr, size_t new_size) {
  uintptr_t offset = (uintptr_t)((uint8_t*)ptr)[-1];
  uintptr_t original = ((uintptr_t) ptr) - offset;
  uintptr_t new_ptr = (uintptr_t)realloc((void*)original, new_size + SZ_VAL);
  if((void*)new_ptr == NULL) return NULL;
  uintptr_t aligned = (new_ptr + SZ_VAL) & ~(SZ_VAL - 1);
  ((uint8_t*)aligned)[-1] = (uint8_t)(aligned - new_ptr);
  return (void*)aligned;
}
