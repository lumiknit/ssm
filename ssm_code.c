#include "ssm_config.h"
#include "ssm_alloc.h"

#include "ssm_code.h"

#include <stdio.h>

/* OpCode */

int is_opcode_with_global(opcode_t o) {
  switch(o) {
  case OP_ACC_G:
    return 1;
  default:
    return 0;
  }
}

int is_opcode_With_int(opcode_t o) {
  switch(o) {
  case OP_JMP:
    return 1;
  default:
    return 0;
  }
}

/* Code Set*/

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
  set->codes = NULL;
}

#define log_err(ret, ...) { fprintf(stderr, __VA_ARGS__); return (ret); }

int load_code(code_set_t *set, size_t size, uint8_t *chunk) {
  /* Check capacity of array and extend the array if we need */
  if(set->num_codes >= set->cap_codes) {
    set->cap_codes <<= 1;
    void *p = realloc(set->codes, sizeof(code_t) * set->cap_codes);
    if(p == NULL) {
      log_err(-1, "Failed to allocate code array!\n");
    }
    set->codes = (code_t*) p;
  }

  code_t *pc = NULL; /* Previous code */
  if(set->num_codes > 0) pc = set->codes[set->num_codes - 1];
  code_t *cc = &set->codes[set->num_codes]; /* Current code */

  /* -- Read haheader -- */

  /* Check the chunk size */
  const size_t hd_size = 4 * 8;
  if(size < hd_size) {
    log_err(-2, "Chunk size is smaller than header size!\n");
  }

  uint32_t *hd = (uint32_t*) chunk;

  /* Check magic number */
  if(!(chunk[0] == 0xca && chunk[1] == 0xfe &&
       chunk[2] == 0x53 && chunk[3] == 0x01)) {
    log_err(-3, "Wrong magic number!\n");
  }

  /* Check token */
  cc->prev_token = hd[1];
  cc->curr_token = hd[2];
  if(pc != NULL && cc->prev_token != pc->curr_token) {
    log_err(-4, "Wrong token!\n");
  }

  /* Check global offset */
  cc->off_global = hd[3];
  if((pc == NULL && cc->off_global != 0) ||
     (pc != NULL && cc->off_global != pc->off_global + pc->num_globals)) {
    log_err(-5, "Wrong global offset!\n");
  }

  /* Read the rest headers */
  cc->num_globals = hd[4];
  const uint32_t sz_global = hd[5];
  cc->num_ops = hd[6];
  const uint32_t sz_opcode = hd[7];

  /* Check chunk size with size infomation */
  if(size != hd_size + sz_global + sz_opcode) {
    log_err(-6, "Chunk size is different with denoted one!\n");
  }

  /* -- Read globals -- */
  uint32_t gp, gi = 0;

  /* Allocate global slots */
  cc->globals = (val_t*) a_alloc(sizeof(val_t) * cc->num_globals);
  if(cc->globals == NULL) {
    log_err(-10, "Cannot allocate global slots!\n");
  }

  /* Scan pool size */
  size_t pool_size = 0;
  for(gp = hd_size; gp < hd_size + sz_global;) {
    uint8_t kind = bytes[gp++];
    switch(kind) {
    case 0x00: {
      uint32_t size = *(uint32_t*)(bytes + gp);
      gp += 4 + size;
      size = (size + SZ_VAL - 1) / SZ_VAL;
      pool_size += 1 + size;
    } break;
    case 0x01: gp += 4; break;
    case 0x02: gp += 4; break;
    default:
      log_err(-8, "Unexpected kind of global constant!\n");
    }
  }
  cc->pool = (val_t*) a_alloc(SZ_VAL * pool_size);
  if(cc->pool == NULL) {
    log_err(-13, "Cannot Allocate Pool\n");
  }

  /* Read globals */
  uint32_t pi = 0;
  for(gp = hd_size; gp < hd_size + sz_global;) {
    if(gi >= cc->num_globals) {
      log_err(-7, "Too many global elements!\n");
    }
    uint8_t kind = bytes[gp++];
    switch(kind) {
    case 0x00: { /* Bytes */
      uint32_t size = *(uint32_t*)(bytes + gp);
      gp += 4;
      const uint32_t asize = (size + SZ_VAL - 1) / SZ_VAL;
      cc->globals[gi] = ptr_to_val(cc->pool + pi);
      cc->pool[pi++] = hd_build_large(asize);
      cc->pool[pi + asize - 1] = 0;
      memcpy(cc->pool + pi, size, bytes + gp);
      pi += asize;
    } break;
    case 0x01: /* Int */
      cc->globals[gi] = int_to_val(*(int32_t*)(bytes + gp));
      gp += 4;
      break;
    case 0x02: /* Float */
      cc->globals[gi] = flt_to_val(*(float*)(bytes + gp));
      gp += 4;
      break;
    default:
      log_err(-8, "Unexpected kind of global constant!\n");
    }
    gi++;
  }
  if(gp >= hd_size + sz_global) {
    log_err(-8, "Too many bytes read for global segment!\n");
  }

  /* -- Read opcodes -- */
  uint32_t op = hd_size + sz_global;
  uint32_t oi = 0;

  /* Allocate label table */
#error UNIMPLEMENTED!

  return 0;
}
