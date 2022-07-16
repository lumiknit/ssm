#ifndef __SSM_VAL_H__
#define __SSM_VAL_H__

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

/* Pointer size types */
typedef intptr_t iptr_t;
typedef uintptr_t uptr_t;
typedef void* ptr_t;

#if UINTPTR_MAX >= 0xffffffffffffffff /* 8-byte */

#define SZ_VAL ((size_t)8)
#define BITS_VAL ((size_t)64)
typedef double fptr_t;

#else /* <= 4-byte */

#define SZ_VAL ((size_t)4)
#define BITS_VAL ((size_t)32)
typedef float fptr_t;

#endif

union vptr {
  void *p;
  iptr_t i;
  uptr_t u;
  fptr_t f;
};

/* Value */
typedef iptr_t val_t;

/* Value Representation
 *  * Int: LSB = 1, value is shifted to the right by 1
 *  * Float: LSB = 1
 *  * Pointer: aligned pointer, but LSB is makred by 1
 *  * Tagged Tuple: aligned pointer
 */

/* Val type checker */
#define is_lit_val(x) ((x) & 1)
#define is_tup_val(x) (!is_lit_val(x))

/* Int <-> Val Conversion */
#define val_to_int(x) ((iptr_t)(x)) >> 1)
#define int_to_val(i) ((((val_t)(i)) << 1) | (val_t)1)

/* Ptr <-> Val Conversion */
#define val_to_ptr(x) ((void*)((uptr_t)(x) & ~(val_t)1))
#define ptr_to_val(x) ((val_t)(i) | (val_t)1)

/* Flt <-> Val Conversion */
#define val_to_flt(x) (((union vptr)(((val_t)(x)) & ~((val_t)1))).f)
#define flt_to_val(f) (((union vptr)(f)).i | (val_t)1)

/* Tup <-> Val Conversion */
#define val_to_tup(x) ((val_t*)(x))
#define tup_to_val(x) ((val_t*)(x))

/* Header */
typedef uptr_t hd_t;

/* Header Representation:
 *
 * [32-bit]
 * | <- high                               low -> |
 *   * Small object (Tagged tuple)
 * | color (2b) | 0 (1b) | size (13b) | tag (16b) |
 *   * Large object (Raw bytes)
 * | color (2b) | 1 (1b) |       size  (29b)      |
 *
 * [64-bit]
 * | <- high                               low -> |
 *   * Small object (Tagged tuple)
 * | color (2b) | 0 (1b) | size (45b) | tag (16b) |
 *   * Large object (Raw bytes)
 * | color (2b) | 1 (1b) |       size  (61b)      |
 */

#define HD_COLOR_MASK (((hd_t)0x3) << (BITS_VAL - 2))
#define HD_TYPE_MASK (((hd_t)0x1) << (BITS_VAL - 3))
#define HD_TAG_MASK ((hd_t)0xffff)
#define HD_SMALL_SIZE_MASK (~(HD_COLOR_MASK | HD_TYPE_MASK | HD_TAG_MASK))
#define HD_LARGE_SIZE_MASK (~(HD_COLOR_MASK | HD_TYPE_MASK))

#define COLOR_L (((hd_t)0x1) << (BITS_VAL - 2))
#define COLOR_H (((hd_t)0x1) << (BITS_VAL - 1))
#define COLOR_A HD_COLOR_MASK

/* Header Info Extract */
#define hd_color(h) (((hd_t)(h)) & HD_COLOR_MASK)
#define hd_type(h) (((hd_t)(h)) & HD_TYPE_MASK)
#define hd_tag(h) (((hd_t)(h)) & HD_TAG_MASK)
#define hd_small_size(h) ((((hd_t)(h)) & HD_SMALL_SIZE_MASK) >> 16)
#define hd_large_size(h) (((hd_t)(h)) & HD_LARGE_SIZE_MASK)

/* Header Build Helper */
#define hd_build_tag(tag) ((hd_t)(tag) & HD_TAG_MASK)
#define hd_build_color(color) (((hd_t)(color)) << (BITS_VAL - 2))
#define hd_build_small_size(size) ((((hd_t)(size)) << 16) & HD_SMALL_SIZE_MASK)
#define hd_build_large_size(size) (((hd_t)(size)) & HD_LARGE_SIZE_MASK)
#define hd_build_small(color, tag, size) \
  (hd_build_tag(tag) | hd_build_color(color) | hd_build_small_size(size))
#define hd_build_large(color, size) \
  (HD_TYPE_MASK | hd_build_color(color) | hd_build_large_size(size))

/* Header Color Helper */
#define hd_mark_l(h) h |= COLOR_L
#define hd_mark_h(h) h |= COLOR_H
#define hd_mark(h) h |= COLOR_A
#define hd_unmark_l(h) h &= ~COLOR_L
#define hd_unmark_h(h) h &= ~COLOR_H
#define hd_unmark(h) h &= ~COLOR_A
#define hd_color_l(h) (h & COLOR_L)
#define hd_colorhl(h) (h & COLOR_H)

/* OpCode */
typedef int32_t op_t;

#endif
