/**
 * @file ssm_vm.c
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#include <ssm.h>
#include <ssm_i.h>

#include "./ssm_vm_header.c"

// Code and helpers

typedef struct Chunk {
  struct Chunk *next;
  size_t n_code;
  ssmOp ops[1];
} Chunk;

static Chunk* allocChunk(size_t size) {
  const size_t bytes = sizeof(Chunk) + size;
  Chunk *c = aligned_alloc(SSM_WORD_SIZE, bytes);
  c->next = NULL;
  c->n_code = size;
  return c;
}

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
  Chunk *c = allocChunk(size);
  fread(c->ops, 1, size, file);

  // Close file
  fclose(file);
  return c;
}

static Chunk* openChunkFromMemory(
  const char *data,
  size_t size
) {
  Chunk *c = allocChunk(size);
  memcpy(c->ops, data, size);
  return c;
}

static const char* verifyChunk(ssmVM *vm, Chunk *c) {
  // First allocate same size bytes for marking
  uint8_t *mark = malloc(c->n_code);
  if(mark == NULL) {
    return "SSMVeriCh: cannot allocate mark";
  }
  
  // Return value. When return, use goto L_ret
  const char *ret = NULL;

  // Initialize marks
  memset(mark, 0, c->n_code);

  // Marks
  #define M_OP 0x01
  #define M_JMP_TARGET 0x02
  #define M_X_FN 0x04
  #define M_FN_TARGET 0x08

  // Check header

  // Check opcodes
  size_t i;
  for(i = 0; i < c->n_code;) {
    ssmOp op = c->ops[i];
    // Check header appears only at the beginning
    if(i > 0 && op == SSM_OP_HEADER) {
      ret = "SSMVeriCh: header appears not at the beginning";
      goto L_ret;
    }
    // Mark ad op
    mark[i] |= M_OP;
    // Next loop
    #include "./ssm_vm_verify_loop.c"
  }

  // Check code size
  if(i != c->n_code) {
    ret = "SSMVeriCh: code size mismatch";
    goto L_ret;
  }

  // Check by marks
  for(size_t i = 0; i < c->n_code;) {
    // Jump target must be an opcode
    if(mark[i] & M_JMP_TARGET && !(mark[i] & M_OP)) {
      ret = "SSMVeriCh: jump target is not an opcode";
      goto L_ret;
    }
    // Function target must be x_fn
    if(mark[i] & M_FN_TARGET && !(mark[i] & M_X_FN)) {
      ret = "SSMVeriCh: function target is not x_fn";
      goto L_ret;
    }
  }

  // Undef marks
  #undef M_OP
  #undef M_JMP_TARGET
  #undef M_X_FN
  #undef M_FN_TARGET

L_ret:
  // Finalize
  free(mark);
  return ret;
L_err_magic:
  ret = "SSMVeriCh: invalid magic";
  goto L_ret;
L_err_op:
  ret = "SSMVeriCh: invalid opcode";
  goto L_ret;
L_err_offset:
  ret = "SSMVeriCh: offset points to out of chunk";
  goto L_ret;
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
    // Using direct threading
    #define OP(op) L_op_##op
    #define NEXT(n) goto *jump_table[*(ip += n)]

    static void *jump_table[] =
      #include "./ssm_vm_jmptbl.c"
    ;

  #else
    #define OP(op) case op
    #define NEXT(n) ip += n; break
  #endif

  #ifdef THREADED_CODE
    NEXT(0);
  #else
    for(;;) {
      switch(*ip) {
  #endif

  #include "./ssm_vm_switch.c"
    
  #ifdef THREADED_CODE
  #else
    }
  }
  #endif
}

// --- VM Interpreter END ---
