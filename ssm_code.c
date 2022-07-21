#include "ssm_config.h"
#include "ssm_alloc.h"

#include "ssm_code.h"

#include <stdio.h>
#include <string.h>

/* OpCode */

opcode_param_t opcode_param_type(opcode_t o) {
  switch(o) {
  case OP_ACC:
    return OP_P_INT;
  case OP_ACC_G:
    return OP_P_GLOBAL;
  case OP_JMP:
    return OP_P_LABEL;
  default:
    return OP_P_NONE;
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

void* get_global_by_index(code_set_t *set, uint32_t idx) {
  /* TODO: Change it to binary search */
  int i;
  for(i = 0; i < set->num_codes; i++) {
    uint32_t fr = set->codes[i].off_global;
    uint32_t to = fr + set->codes[i].num_globals;
    if(fr <= idx && idx < to) {
      return set->codes[i].globals + (idx - fr);
    }
  }
  return NULL;
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
  if(set->num_codes > 0) pc = &set->codes[set->num_codes - 1];
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
    uint8_t kind = chunk[gp++];
    switch(kind) {
    case 0x00: {
      uint32_t size = *(uint32_t*)(chunk + gp);
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
    uint8_t kind = chunk[gp++];
    switch(kind) {
    case 0x00: { /* Bytes */
      uint32_t size = *(uint32_t*)(chunk + gp);
      gp += 4;
      const uint32_t asize = (size + SZ_VAL - 1) / SZ_VAL;
      cc->globals[gi] = obj_to_val(cc->pool + pi);
      cc->pool[pi++] = hd_build_large(0, asize);
      cc->pool[pi + asize - 1] = 0;
      memcpy(cc->pool + pi, chunk + gp, size);
      pi += asize;
    } break;
    case 0x01: /* Int */
      cc->globals[gi] = int_to_val(*(int32_t*)(chunk + gp));
      gp += 4;
      break;
    case 0x02: /* Float */
      cc->globals[gi] = flt_to_val(*(float*)(chunk + gp));
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
  uint32_t oi, op;

  /* Allocate label table */
  void **ltbl = (void**) malloc(sizeof(void*) * cc->num_ops);
  if(ltbl == NULL) {
    log_err(-23, "Cannot allocate label table!\n");
  }

  /* Construct label table */
  uint32_t n_ptr = 0, n_int = 0;
  uint8_t *opp = (uint8_t*) cc->op;
  for(oi = 0, op = hd_size + sz_global;
      op < hd_size + sz_global + sz_opcode;
      oi++) {
    ltbl[oi] = opp;
    switch(opcode_param_type(chunk[op])) {
    case OP_P_NONE:
      op += 1;
      opp += sizeof(op_t);
      break;
    case OP_P_INT:
      n_int += 1;
      op += 1 + 4;
      opp += sizeof(op_t) + sizeof(op_int_t);
      break;
    case OP_P_GLOBAL:
    case OP_P_LABEL:
      n_ptr += 1;
      op += 1 + 4;
      opp += sizeof(op_t) + sizeof(op_ptr_t);
      break;
    }
  }
  if(op >= hd_size + sz_global + sz_opcode) {
    log_err(-8, "Too many bytes read for opcode segment!\n");
  }
  if(oi != cc->num_ops) {
    log_err(-8, "Wrong number of opcodes!\n");
  }

  /* Allocate ops */
  const uint32_t n_op = cc->num_ops;
  const size_t sz_op =
    sizeof(op_t) * n_op +
    sizeof(op_int_t) * n_int +
    sizeof(op_ptr_t) * n_ptr;
  cc->op = (op_t*) a_alloc(sz_op);
  if(cc->op == NULL) {
    log_err(-22, "Cannot allocate code segment!\n");
  }

  /* Read opcodes */
  uint8_t *dst = (uint8_t*) cc->op;
  for(op = hd_size + sz_global;
      op < hd_size + sz_global + sz_opcode;) {
    const op_t c = chunk[op];
    *(op_t*) dst = c;
    dst += sizeof(op_t);
    switch(opcode_param_type(c)) {
    case OP_P_NONE: op += 1; break;
    case OP_P_INT: {
      *(int32_t*) dst = *(int32_t*) (chunk + (op + 1));
      dst += sizeof(op_int_t);
      op += 1 + 4;
    } break;
    case OP_P_GLOBAL: {
      uint32_t idx = *(uint32_t*) (chunk + (op + 1));
      if(cc->off_global <= idx) { /* Load from current code */
        if(cc->off_global + cc->num_globals <= idx) {
          log_err(-22, "Global index out of range (%d)!\n", idx);
        }
        *(op_ptr_t*) dst = cc->globals + (idx - cc->off_global);
      } else { /* Load from previous codes */
        *(op_ptr_t*) dst = get_global_by_index(set, idx);
      }
      dst += sizeof(op_ptr_t);
      op += 1 + 4;
    } break;
    case OP_P_LABEL: {
      uint32_t idx = *(uint32_t*) (chunk + (op + 1));
      *(op_ptr_t*) dst = ltbl[idx];
      dst += sizeof(op_ptr_t);
      op += 1 + 4;
    } break;
    }
  }
  if(op >= hd_size + sz_global + sz_opcode) {
    log_err(-8, "Too many bytes read for opcode segment!\n");
  }

  /* Free label table */
  free(ltbl);

  /* Load Done */
  set->num_codes += 1;
  return set->num_codes;
}
