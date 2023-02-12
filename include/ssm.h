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


// Pointer-size values

typedef intptr_t ssmIptr;
typedef uintptr_t ssmUptr;

#if UINTPTR_MAX == 0xffffffffffffffffu // 64-bit

#define SSM_WORD_BITS 64
#define SSM_WORD_SIZE 8
#define SSM_FPTR 1
typedef double ssmFptr;

#elif UINTPTR_MAX == 0xffffffff // 32-bit

#define SSM_WORD_BITS 32
#define SSM_WORD_SIZE 4
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

#define ssmVal2Int(v) (((ssmIptr)v) >> 1)
#define ssmVal2Uint(v) (((ssmUptr)v) >> 1)
#define ssmVal2Flt(v) (((ssmFptr*)(((ssmUptr)v) & ~(ssmUptr)1)))
#define ssmVal2Ptr(T, v) (((T*)(((ssmUptr)v) & ~(ssmUptr)1)))
#define ssmVal2Tup(v) ((ssmT)v)

#define ssmInt2Val(i) ((((ssmV)i) << 1) | 1)
#define ssmUint2Val(u) ((((ssmV)u) << 1) | 1)
#define ssmFlt2Val(f) (((ssmV)f) | 1)
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
#define ssmHdLongWords(v) (ssmHdLongSize(v) / SSM_WORD_SIZE)
#define ssmHdShortBytes(v) (ssmHdSize(v) * SSM_WORD_SIZE)
#define ssmHdShortWords(v) ssmHdSize(v)

#define ssmHdBytes(v) (ssmHdIsLong(v) ? ssmHdLongBytes(v) : ssmHdShortBytes(v))
#define ssmHdWords(v) (ssmHdIsLong(v) ? ssmHdLongWords(v) : ssmHdShortWords(v))

#define ssmHdUnmarked(v) ((v) & ~SSM_COLOR_MASK)
#define ssmHdMarked(v) ((v) | SSM_COLOR_MASK)

#define ssmLongHd(size) (SSM_LONG_BIT | (((ssmV)size) & SSM_LONG_SIZE_MASK))
#define ssmShortHd(size, tag) \
  ((((ssmV)size) & SSM_LONG_SIZE_MASK) | (((ssmV)tag) & SSM_TAG_MASK))


// Tuple Helpers

#define ssmTHd(t) ((ssmT)t)[0]
#define ssmTNext(t) ((ssmT*)t)[-1]
#define ssmTElem(t, i) ((ssmT)t)[i + 1]
#define ssmTByte(t, i) ((char*)((ssmT)t))[i]

#define ssmTWords(words) (1 + words)
#define ssmTWordsFromBytes(bytes) (1 + ((bytes + SSM_WORD_SIZE - 1) / SSM_WORD_SIZE))


#endif