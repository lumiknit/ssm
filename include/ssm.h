/**
 * @file ssm.h
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#ifndef __SSM_H__
#define __SSM_H__

// Include standard libraries

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Endianess
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
    defined(__LITTLE_ENDIAN__) || \
    defined(__ARMEL__) || \
    defined(__THUMBEL__) || \
    defined(__AARCH64EL__) || \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)

  #define SSM_LITTLE_ENDIAN
  
#endif

// Pointer-size values

typedef intptr_t ssmIptr;
typedef uintptr_t ssmUptr;

#if UINTPTR_MAX == 0xffffffffffffffffu // 64-bit

  #define SSM_WORD_BITS ((size_t)64)
  #define SSM_WORD_SIZE ((size_t)8)
  #define SSM_FPTR 1
  typedef double ssmFptr;

#elif UINTPTR_MAX == 0xffffffff // 32-bit

  #define SSM_WORD_BITS ((size_t)32)
  #define SSM_WORD_SIZE ((size_t)4)
  #define SSM_FPTR 1
  typedef float ssmFptr;

#else // 16-bit

  #error "Not supported pointer size"

#endif // if uintptr_max


// Value type and helpers
typedef ssmUptr ssmV; // Value
typedef ssmV* ssmT; // Tuple

// Int, Uint, Float = Literal numbers
// Ptr = Pointer (not managed by GC) aligned by 2n (n >= 1)
// Tup = Pointer of tuple (managed by GC) aligned by 2n (n >= 1)

typedef union ssmPtrUnion {
  ssmUptr u;
  ssmIptr i;
#ifdef SSM_FPTR
  ssmFptr f;
#endif
  void *ptr;
} ssmPtrUnion;

#define ssmVal2Int(v) (((ssmIptr)v) >> 1)
#define ssmVal2Uint(v) (((ssmUptr)v) >> 1)
#define ssmVal2Flt(v) (((ssmPtrUnion){.u = ((ssmUptr)v) & ~(ssmUptr)1}).f)
#define ssmVal2Ptr(T, v) (((T*)(((ssmUptr)v) & ~(ssmUptr)1)))
#define ssmVal2Tup(v) ((ssmT)v)

#define ssmInt2Val(i) ((((ssmV)i) << 1) | 1)
#define ssmUint2Val(u) ((((ssmV)u) << 1) | 1)
#define ssmFlt2Val(fv) (((ssmPtrUnion){.f = (fv)}).u | 1)
#define ssmPtr2Val(p) (((ssmV)p) | 1)
#define ssmTup2Val(t) ((ssmV)t)

#define ssmIsLiteral(v) (v & 1)
#define ssmIsGCVal(v) (!ssmIsLiteral(v))


// -- GC Header

/* Header Representation
 * [32-bit]
 * | <- high                               low -> |
 *   * Short tuple (Tagged tuple)
 * | color (2b) | 0 (1b) | size (13b) | tag (16b) |
 *   * Long tuple (Raw bytes)
 * | color (2b) | 1 (1b) |       size  (29b)      |
 *
 * [64-bit]
 * | <- high                               low -> |
 *   * Short tuple (Tagged tuple)
 * | color (2b) | 0 (1b) | size (45b) | tag (16b) |
 *   * Long tuple (Raw bytes)
 * | color (2b) | 1 (1b) |       size  (61b)      |
 * 
 * Note that the sizes of short tuples is in words
 * and the sizes of long tuples is in bytes.
 */

#define SSM_V_SHIFT(v, n) (((ssmV)v) << n)

#define SSM_COLOR_SHIFT (SSM_WORD_BITS - 2)
#define SSM_COLOR_WHITE SSM_V_SHIFT(0, SSM_COLOR_SHIFT)
#define SSM_COLOR_GRAY SSM_V_SHIFT(1, SSM_COLOR_SHIFT)
#define SSM_COLOR_RED SSM_V_SHIFT(2, SSM_COLOR_SHIFT)
#define SSM_COLOR_BLACK SSM_V_SHIFT(3, SSM_COLOR_SHIFT)
#define SSM_COLOR_MASK SSM_V_SHIFT(3, SSM_COLOR_SHIFT)

#define SSM_LONG_SHIFT (SSM_WORD_BITS - 3)
#define SSM_LONG_BIT SSM_V_SHIFT(1, SSM_LONG_SHIFT)

#define SSM_LONG_SIZE_MASK (~(SSM_COLOR_MASK | SSM_LONG_BIT))

#define SSM_TAG_MASK ((ssmV)0xffff)
#define SSM_SIZE_SHIFT 16
#define SSM_SIZE_MASK (~(SSM_COLOR_MASK | SSM_LONG_BIT | SSM_TAG_MASK))

#define ssmHdColor(v) ((v) & SSM_COLOR_MASK)
#define ssmHdIsLong(v) ((v) & SSM_LONG_BIT)
#define ssmHdLongSize(v) ((v) & SSM_LONG_SIZE_MASK)
#define ssmHdTag(v) ((v) & SSM_TAG_MASK)
#define ssmHdSize(v) (((v) & SSM_SIZE_MASK) >> SSM_SIZE_SHIFT)

#define ssmHdLongBytes(v) ssmHdLongSize(v)
#define ssmHdLongWords(v) \
  ((ssmHdLongSize(v) + SSM_WORD_SIZE - 1) / SSM_WORD_SIZE)
#define ssmHdShortBytes(v) (ssmHdSize(v) * SSM_WORD_SIZE)
#define ssmHdShortWords(v) ssmHdSize(v)

#define ssmHdBytes(v) (ssmHdIsLong(v) ? ssmHdLongBytes(v) : ssmHdShortBytes(v))
#define ssmHdWords(v) (ssmHdIsLong(v) ? ssmHdLongWords(v) : ssmHdShortWords(v))

#define ssmHdUnmarked(v) ((v) & ~SSM_COLOR_MASK)
#define ssmHdMarked(v) ((v) | SSM_COLOR_MASK)

#define ssmLongHd(size) (SSM_LONG_BIT | (((ssmV)size) & SSM_LONG_SIZE_MASK))
#define ssmShortHd(tag, size) \
  (((((ssmV)size) << SSM_SIZE_SHIFT) & SSM_SIZE_MASK) | \
   (((ssmV)tag) & SSM_TAG_MASK))


// Tuple Helpers

#define ssmTHd(t) ((ssmT)t)[0]
#define ssmTMarkList(t) ((ssmT*)t)[-1]
#define ssmTNext(t) ((ssmT*)t)[-2]
#define ssmTElem(t, i) ((ssmT)t)[i + 1]
#define ssmTByte(t, i) ((char*)((ssmT)t))[i]

#define ssmTWords(words) (1 + words)
#define ssmTWordsFromBytes(bytes) \
  (1 + ((bytes + SSM_WORD_SIZE - 1) / SSM_WORD_SIZE))
#define SSM_MINOR_TUP_EXTRA_WORDS 1
#define SSM_MAJOR_TUP_EXTRA_WORDS 2


// -- Stack

typedef struct ssmStack {
  size_t size;
  size_t top;
  ssmV vals[1];
} ssmStack;

// If r2l is true, the sp moves from right to left (as call stack)
// and must use pushStackR and popStackR
ssmStack* ssmNewStack(size_t size, int r2l);
#define ssmFreeStack(stack) free(stack)
// Note: extendStack does not guarantee the stack is aligned
ssmStack* ssmExtendStackToRight(ssmStack* stack, size_t size);
ssmStack* ssmExtendStackToLeft(ssmStack* stack, size_t size);
size_t ssmPushStack(ssmStack* stack, ssmV value);
void ssmPushStackForce(ssmStack** stack, ssmV value);
ssmV ssmPopStack(ssmStack* stack);
size_t ssmPushStackR(ssmStack* stack, ssmV value);
ssmV ssmPopStackR(ssmStack* stack);
#define ssmInStack(stack, ptr) \
  ((stack)->vals <= (ptr) && \
  (ptr) <= (stack)->vals + (stack)->size - 1)


// -- Mem

typedef struct ssmMem {
  // Configurations
  size_t major_gc_threshold_percent;

  // Minor heap
  ssmStack* minor;

  // Major heap
  size_t major_allocated_words;
  size_t major_gc_threshold_words;

  // Major tuple list
#define SSM_MAJOR_LIST_KINDS 3
#define SSM_MAJOR_LIST_IMMORTAL 0 // Never freed
#define SSM_MAJOR_LIST_LEAVES 1 // Cannot contain any reference
#define SSM_MAJOR_LIST_NODES 2 // May contain references
  ssmV *major_list[3];
  
  // Stack
  ssmStack* stack;
  ssmStack* global;

  // Statistics
  size_t minor_gc_count;
  size_t major_gc_count;

  // GC template variables
  ssmT mark_list;
  ssmT write_barrier;
} ssmMem;

void ssmInitMem(ssmMem* mem,
  size_t minor_heap_words,
  size_t major_gc_threshold_percent,
  size_t stack_size,
  size_t global_size);
void ssmFiniMem(ssmMem* mem);

void ssmUpdateMajorGCThreshold(ssmMem* mem);

int ssmFullGC(ssmMem* mem);
int ssmMinorGC(ssmMem* mem);

ssmT ssmNewLongTup(ssmMem *mem, ssmV bytes);
ssmT ssmNewTup(ssmMem *mem, ssmV tag, ssmV words);

void ssmGCWriteBarrier(ssmMem *mem, ssmT tup);

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

  size_t n_chunks;
  void *chunks;
} ssmVM;

void ssmInitVM(ssmVM* vm, ssmConfig* config);
void ssmFiniVM(ssmVM* vm);

int ssmLoadFile(ssmVM *vm, const char *filename);
int ssmLoadString(ssmVM *vm, size_t size, const ssmOp *code);

#endif