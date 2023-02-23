/**
 * @file ssm_vm.c
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#include <ssm.h>
#include <ssm_i.h>

void ssmLoadDefaultConfig(ssmConfig* config) {
  // 100%
  config->major_gc_threshold_percent = 100;
  // <= 1MB
  config->minor_heap_size = (2 << 20) >> 3;
  // <= 1MB
  config->initial_stack_size = (2 << 20) >> 3;
  config->initial_global_size = 128;
}

void ssmInitVM(ssmVM* vm, ssmConfig* config) {
  ssmInitMem(
    &vm->mem,
    config->minor_heap_size,
    config->major_gc_threshold_percent,
    config->initial_stack_size,
    config->initial_global_size);

  vm->code = calloc(1024, 1);
  vm->n_code = 1024;
}

void ssmFiniVM(ssmVM* vm) {
  ssmFiniMem(&vm->mem);
}

#define THREADED_CODE

// --- VM Interpreter BEGIN ---
void ssmRunVM(ssmVM* vm, ssmV entry_ip) {
  ssmOp *ip = vm->code + entry_ip;

  // Initialize macros
#ifdef THREADED_CODE
  #define OP(op) L_op_##op
  #define NEXT(n) goto *jump_table[*(ip += n)]
#else
  #define OP(op) case op
  #define NEXT(n) ip += n; break;
#endif

#ifdef THREADED_CODE
  static void *jump_table[] = {
    &&L_op_NOP,
  };
  NEXT(0);
#else
  for(;;) { switch(*ip) {
#endif

  OP(NOP):
    printf("NOP %p\n", ip);
    NEXT(1);
    
#ifdef THREADED_CODE
#else
  } }
#endif
}
// --- VM Interpreter END ---
