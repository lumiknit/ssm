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

#include <ssm_ops.h>

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
  void *code;
} ssmVM;

void ssmInitVM(ssmVM* vm, ssmConfig* config);
void ssmFiniVM(ssmVM* vm);

int ssmLoadFile(ssmVM *vm, const char *filename);
int ssmLoadString(ssmVM *vm, const ssmOp *code, size_t n_code);

void ssmRunVM(ssmVM* vm, ssmV entry_ip);

#endif