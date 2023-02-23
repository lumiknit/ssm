/**
 * @file ssm.h
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#ifndef __SSM_H__
#define __SSM_H__

#include <ssm_rt.h>

// Bytecode

typedef uint8_t ssmOp;
enum ssmOpCode {
  // Special
  SSM_OP_NOP = 0,
  SSM_OP_MAGIC,
  // Stack
  SSM_OP_PUSH,
  SSM_OP_POP,
  SSM_OP_SET,
  SSM_OP_JMP,
  // Global
  SSM_OP_SETGLOBAL,
  SSM_OP_GETGLOBAL,
  // Call
  SSM_OP_IP,
  SSM_OP_CALL,
  SSM_OP_RET,
  SSM_OP_TAILCALL,
  // Tup
  SSM_OP_TUP,
  SSM_OP_TAG,
  SSM_OP_SIZE,
  // Int op
  SSM_OP_I_ADD,
  SSM_OP_I_SUB,
  SSM_OP_I_MUL,
  SSM_OP_I_DIV,
  SSM_OP_I_MOD,
  SSM_OP_I_UNM,
  // Flt op
  SSM_OP_F_ADD,
  SSM_OP_F_SUB,
  SSM_OP_F_MUL,
  SSM_OP_F_DIV,
  SSM_OP_F_MOD,
  SSM_OP_F_UNM,
  // Long op
  SSM_OP_L_TUP,
  SSM_OP_L_SIZE,
  SSM_OP_L_AT,
};

// VM

typedef struct ssmConfig {
  size_t minor_heap_size;
  size_t major_gc_threshold_percent;
  size_t initial_stack_size;
  size_t initial_global_size;
} ssmConfig;

void ssmLoadDefaultConfig(ssmConfig* config);

typedef struct ssmVM {
  ssmMem mem;

  size_t n_code;
  ssmOp *code;
} ssmVM;

void ssmInitVM(ssmVM* vm, ssmConfig* config);
void ssmFiniVM(ssmVM* vm);

void ssmRunVM(ssmVM* vm, ssmV entry_ip);

#endif