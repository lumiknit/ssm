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
  size_t size;
  ssmOp bytes[1];
} Chunk;

static Chunk* allocChunk(size_t size) {
  const size_t bytes = sizeof(Chunk) + size;
  Chunk *c = aligned_alloc(SSM_WORD_SIZE, bytes);
  c->next = NULL;
  c->size = size;
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
  fread(c->bytes, 1, size, file);

  // Close file
  fclose(file);
  return c;
}

static Chunk* openChunkFromMemory(
  const char *data,
  size_t size
) {
  Chunk *c = allocChunk(size);
  memcpy(c->bytes, data, size);
  return c;
}

static const char* verifyChunk(ssmVM *vm, Chunk *c) {
  // First allocate same size bytes for marking
  uint8_t *mark = malloc(c->size);
  if(mark == NULL) {
    return "SSMVeriCh: cannot allocate mark";
  }
  
  // Return value. When return, use goto L_ret
  const char *ret = NULL;

  // Initialize marks
  memset(mark, 0, c->size);

  // Marks
  #define M_OP 0x01
  #define M_JMP_TARGET 0x02
  #define M_X_FN 0x04
  #define M_FN_TARGET 0x08

  // Check header
  OpHeader hd;
  if(readOpHeader(&hd, c->bytes) == 0) {
    ret = "SSMVeriCh: cannot read header";
    goto L_ret;
  }
  if(hd.chunk_size != c->size) {
    ret = "SSMVeriCh: chunk size mismatch";
    goto L_ret;
  }
  if(hd.global_offset != vm->mem.global->top) {
    ret = "SSMVeriCh: global offset mismatch";
    goto L_ret;
  }
  const size_t n_globals = hd.global_offset + hd.global_count;

  // Check opcodes
  size_t i;
  for(i = 0; i < c->size;) {
    ssmOp op = c->bytes[i];
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
  if(i != c->size) {
    ret = "SSMVeriCh: code size mismatch";
    goto L_ret;
  }

  // Check by marks
  for(size_t i = 0; i < c->size; i++) {
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
L_err_global:
  ret = "SSMVeriCh: global points to out of chunk";
  goto L_ret;
L_err_left_aligned:
  ret = "SSMVeriCh: some opcodes must be left aligned";
  goto L_ret;
L_err_right_aligned:
  ret = "SSMVeriCh: some opcodes must be right aligned";
  goto L_ret;
}

// --- VM Initialization

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

  vm->chunks = NULL;
  vm->n_chunks = 0;
}

void ssmFiniVM(ssmVM* vm) {
  // Free all codes
  Chunk *c = vm->chunks;
  while(c != NULL) {
    Chunk *next = c->next;
    free(c);
    c = next;
  }

  ssmFiniMem(&vm->mem);
}

// --- Load file and interp

static int ssmLoadChunk(ssmVM *vm, Chunk *c);
static int ssmRunVM(ssmVM *vm, Chunk *c);

int ssmLoadFile(ssmVM *vm, const char *filename) {
  // Read file
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    return -1;
  }
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);
  
  // Align size
  size_t a_size = (size + sizeof(Chunk) + SSM_WORD_SIZE - 1) & ~(SSM_WORD_SIZE - 1);
  if(a_size <= size) {
    // Overflow
    return -1;
  }

  // Allocate Chunk
  Chunk *c = aligned_alloc(SSM_WORD_SIZE, a_size);
  c->next = NULL;
  c->size = size;
  fread(c->bytes, 1, size, file);

  // Run VM
  return ssmLoadChunk(vm, c);
}

int ssmLoadString(ssmVM *vm, size_t size, const ssmOp *code) {
  // Align size
  size_t a_size = (size + sizeof(Chunk) + SSM_WORD_SIZE - 1) & ~(SSM_WORD_SIZE - 1);
  if(a_size <= size) {
    // Overflow
    return -1;
  }

  // Allocate code
  Chunk *c = aligned_alloc(SSM_WORD_SIZE, a_size);
  c->next = NULL;
  c->size = size;
  memcpy(c->bytes, code, size);

  // Run VM
  ssmLoadChunk(vm, c);
  return 0;
}

static int ssmLoadChunk(ssmVM *vm, Chunk *c) {
  // Verify chunk
  const char *err = verifyChunk(vm, c);
  if(err != NULL) {
    fprintf(stderr, "SSM: %s", err);
    return -1;
  }

  // Run via VM
  return ssmRunVM(vm, c);
}

#define THREADED_CODE

// --- VM Interpreter BEGIN ---

#define BP2AP(bp) ((bp) + sizeof(ssmReg) / sizeof(void*))

static int ssmRunVM(ssmVM* vm, Chunk *c) {
  // Initialize

  // Create aligned zero for dummy function
  union aligned_zero {
    uint8_t u8[16];
  } aligned_zero_pack = {0};
  uint8_t *aligned_zero = aligned_zero_pack.u8;
  if((uintptr_t)aligned_zero % 2 != 0) {
    aligned_zero += 1;
  }

  // Push chunk
  c->next = vm->chunks;
  vm->chunks = c;
  vm->n_chunks++;

  // Create registers
  ssmReg reg = {
    .ip = c->bytes,
    .sp = vm->mem.stack->vals + vm->mem.stack->top,
    .bp = vm->mem.stack->vals + vm->mem.stack->top,
  };

  { // Parse header
    OpHeader h;
    int read = readOpHeader(&h, reg.ip);
    reg.ip += read;

    // Change globals
    size_t new_size = h.global_offset + h.global_count;
    if(new_size > vm->mem.global->size) {
      ssmExtendStackToRight(vm->mem.global, new_size);
    }
    vm->mem.global->top = new_size;
  }

  // Helper Macros
  // Pointer must be pushed 
#define PUSH_PTR(p)

  // Push dummy function for return
  *(--reg.sp) = ssmPtr2Val(aligned_zero);
  // Push ip
  *(--reg.sp) = ssmPtr2Val(c->bytes);
  // Push bp
  *(--reg.sp) = ssmUint2Val(vm->mem.stack->vals_hi - reg.bp);

  // Initialize macros
  #ifdef THREADED_CODE
    // Using direct threading
    #define OP(op) L_op_##op
    #define NEXT(n) goto *jump_table[*(reg.ip += n)]

    static void *jump_table[] =
      #include "./ssm_vm_jmptbl.c"
    ;
  #else
    #define OP(op) case op
    #define NEXT(n) reg.ip += n; break
  #endif

  #ifdef THREADED_CODE
    NEXT(0);
  #else
    for(;;) {
      switch(*reg.ip) {
  #endif

  // TODO: Fill switch
  #include "./ssm_vm_switch.c"
    
  #ifdef THREADED_CODE
  #else
    }
  }
  #endif

  return 0;
}

// --- VM Interpreter END ---
