#ifndef __SSM_CODE_H__
#define __SSM_CODE_H__

#include <stdint.h>

#include "ssm_val.h"

/* Opcodes */

/* OpCode */
typedef int32_t op_t;

enum opcode {
  OP_NOP = 0,
};

/* Code set */

typedef struct code {
  /* Globals */
  op_t first_global, num_globals;
  val_t *globals;

  /* Opcodes */
  op_t *op;
} code_t;

typedef struct code_set {
  int num_codes, cap_codes;
  code_t *codes;
} code_set_t;

void init_code_set(code_set_t*);
void fin_code_set(code_set_t*);

int load_code(code_set_t*, size_t size, uint8_t *bytes);

#endif