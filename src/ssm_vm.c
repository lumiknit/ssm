/**
 * @file ssm_vm.c
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#include <ssm.h>
#include <ssm_i.h>

// Code and helpers

typedef struct Chunk {
  struct Code *next;
  size_t n_code;
  ssmOp ops[1];
} Chunk;

static Chunk* openChunkFromFile(const char *filename) {
  // Read file
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    return NULL;
  }
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  // Allocate code
  Chunk *c = malloc(sizeof(Chunk) + size);
  c->next = NULL;
  c->n_code = size;
  fread(c->ops, 1, size, file);
  return c;
}

static int verifyCode(const ssmVM *vm, const Code *c) {
  const uint8_t OP = 0x01;
  const uint8_t JUMP_TARGET = 0x02;
  // TODO: Check headers and VM config
  unimplemented();
  // TODO: Try to allocate same size of memory
  unimplemented();
  // TODO: 1st loop through all instructions
  unimplemented();
  size_t i;
  for(i = 0; i < c->n_code;) {
    ssmOp op = c->ops[i];
    // TODO: Calculate the size of opcode and operands
    unimplemented();
    // TODO: Mark the opcode position as OP
    unimplemented();
    // TODO: Mark the jump target position as JUMP_TARGET
    unimplemented();
  }
  // TODO: 2nd loop
  for(i = 0; i < c->n_code;) {
    // TODO: Check mark
    // If JUMP_TARGET is marked, it must be OP
    unimplemented();
  }
  // DONE
  return 0;
}


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

  vm->code = NULL;
  vm->n_code = 1024;
}

void ssmFiniVM(ssmVM* vm) {
  // Free all codes
  Code *c = vm->code;
  while(c != NULL) {
    Code *next = c->next;
    free(c);
    c = next;
  }

  ssmFiniMem(&vm->mem);
}

int ssmLoadFile(ssmVM *vm, const char *filename) {
  // Read file
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    return -1;
  }
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  // Allocate code
  Code *c = malloc(sizeof(Code) + size);
  c->next = vm->code;
  c->n_code = size;
  fread(c->code, 1, size, file);
  vm->code = (void*) c;

  // Run VM
  ssmRunVM(vm, (ssmOp*) &c->code);
  return 0;
}

int ssmLoadCode(ssmVM *vm, const ssmOp *code, size_t n_code) {
  // Allocate code
  Code *c = malloc(sizeof(Code) + n_code);
  c->next = vm->code;
  c->n_code = n_code;
  memncpy(c->code, code, n_code);
  vm->code = (void*) c;

  // Run VM
  ssmRunVM(vm, (ssmOp*) &c->code);
  return 0;
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
  static void *jump_table[] =
#include "./ssm_vm_jmptbl.c"
  NEXT(0);
#else
  for(;;) { switch(*ip) {
#endif

#include "./ssm_vm_switch.c"
    
#ifdef THREADED_CODE
#else
  } }
#endif
}
// --- VM Interpreter END ---
