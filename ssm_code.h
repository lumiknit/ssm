#ifndef __SSM_CODE_H__
#define __SSM_CODE_H__

#include <stdint.h>

#include "ssm_val.h"

/* Opcodes */

/* OpCode */
typedef int32_t op_t;
typedef int32_t op_int_t;
typedef void* op_ptr_t;

typedef enum opcode {
  OP_NOP = 0,
  OP_ACC_G,
  OP_JMP,
  OP_ACC,
  OP_ACC_0,
} opcode_t;

typedef enum opcode_param {
  OP_P_NONE = 0,
  OP_P_INT = 1,
  OP_P_GLOBAL = 2,
  OP_P_LABEL = 3,
} opcode_param_t;

opcode_param_t opcode_param_type(opcode_t);

/* Code set
 * -- Structure
 * - [00-07] header
 * - [00] (4B) Magic Number: 0xca 0xfe 0x53 0x01
 * - [01] (4B) Previous Token
 * - [02] (4B) Current Token
 * - [03] (4B) Index Offset of Globals
 * - [04] (4B) Number of Globals
 * - [05] (4B) Size of Global Segment
 * - [06] (4B) Number of Opcodes
 * - [07] (4B) Size of Opcode Segment
 * - [08-xx] (x B) Global Segment
 * - [xx-yy] (y B) Opcode Segment
 * -- Global
 * - (1B) Tag + (x B) Alpha
 * - 0x00 (Bytes) + (4B) Size + (Size B) Raw Data
 * - 0x01 (Int) + (4B) Int
 * - 0x02 (Flt) + (4B) Float
 */

typedef struct code {
  /* Token (for continuity check) */
  uint32_t prev_token;
  uint32_t curr_token;

  /* Globals */
  uint32_t off_global; /* Global index offset */
  uint32_t num_globals; /* # of globals */
  val_t *globals;
  val_t *pool; /* Global constant pool */

  /* Opcodes */
  uint32_t num_ops;
  op_t *op;
} code_t;

typedef struct code_set {
  int num_codes, cap_codes;
  code_t *codes;
} code_set_t;

void init_code_set(code_set_t*);
void fin_code_set(code_set_t*);

void* get_global_by_index(code_set_t*, uint32_t);

int load_code(code_set_t*, size_t size, uint8_t *bytes);

#endif
