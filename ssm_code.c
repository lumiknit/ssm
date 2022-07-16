#include "ssm_config.h"
#include "ssm_alloc.h"

#include "ssm_code.h"

#include <stdio.h>

void init_code_set(code_set_t *set) {
  set->num_codes = 0;
  set->cap_codes = CODE_SET_INIT_ENTRIES;
  void *p = malloc(sizeof(code_t) * set->cap_codes);
  if(p == NULL) {
    fprintf(stderr, "Failed to allocate code array!\n");
    exit(1);
  }
  set->codes = (code_t*) p;
}

void fin_code_set(code_set_t *set) {
  free(set->codes);
}

int load_code(code_set_t *set, size_t size, uint8_t *bytes) {
  /* Check capacity of array and extend the array if we need */
  if(set->num_codes >= set->cap_codes) {
    set->cap_codes <<= 1;
    void *p = realloc(set->codes, sizeof(code_t) * set->cap_codes);
    if(p == NULL) {
      fprintf(stderr, "Failed to allocate code array!\n");
      exit(1);
    }
    set->codes = (code_t*) p;
  }
  /* TODO: add load code */
  return 0;
}