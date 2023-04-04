/**
 * @file ssm_i.h
 * @author lumiknit (aasr4r4@gmail.com)
 * @brief ssm internal header
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#ifndef __SSM_I_H__
#define __SSM_I_H__

// Helpers
#define panic(msg) { \
  fprintf(stderr, "[FATAL] (ssm) Panic at %s:%d: %s\n", \
    __FILE__, __LINE__, msg); \
  exit(1); \
}
#define panicf(format, ...) { \
  fprintf(stderr, "[FATAL] (ssm) Panic at %s:%d: " format "\n", \
    __FILE__, __LINE__, __VA_ARGS__); \
  exit(1); \
}
#define unimplemented() { \
  fprintf(stderr, "[FATAL] (ssm) Unimplemented! %s:%d\n", \
    __FILE__, __LINE__); \
  exit(1); \
}

// Logging helper
//#define DEBUG_LOG_ENABLED

#ifdef DEBUG_LOG_ENABLED
#define logf(...) printf(__VA_ARGS__)
#else
#define logf(...) ((void)0)
#endif


// GC

// Logging helper
//#define DEBUG_GC_LOG_ENABLED

#ifdef DEBUG_GC_LOG_ENABLED
#define gcLogf(...) printf(__VA_ARGS__)
#else
#define gcLogf(...) ((void)0)
#endif

#define GC_MIN_MAJOR_HEAP_FACTOR 7


// Endianess

#define shift_le(t, b, idx) (((t)(((uint8_t*)(b))[idx])) << (8 * (idx)))

// read little endian and convert to host endianess

#ifdef SSM_LITTLE_ENDIAN

  // If the host is little endian, we can just read the value directly

  #define read_uint8_t(b) (*(uint8_t*)(b))
  #define read_uint16_t(b) (*(uint16_t*)(b))
  #define read_uint32_t(b) (*(uint32_t*)(b))
  #define read_uint64_t(b) (*(uint64_t*)(b))

  #define read_int8_t(b) (*(int8_t*)(b))
  #define read_int16_t(b) (*(int16_t*)(b))
  #define read_int32_t(b) (*(int32_t*)(b))
  #define read_int64_t(b) (*(int64_t*)(b))

  #define read_float(b) (*(float*)(b))
  #define read_double(b) (*(double*)(b))

#else

  // If the host is unknown endianess, convert value through unsigned int

  #include <stdint.h>

  union word_8 {
    uint8_t u;
    int8_t i;
    uint8_t bytes[1];
  };

  union word_16 {
    uint16_t u;
    int16_t i;
    uint8_t bytes[2];
  };

  union word_32 {
    uint16_t u;
    int16_t i;
    float f;
    uint8_t bytes[4];
  };

  union word_64 {
    uint16_t u;
    int16_t i;
    double f;
    uint8_t bytes[8];
  };

  #define read_uint8_t(b) ( \
    shift_le(uint8_t, b, 0) )

  #define read_uint16_t(b) ( \
    shift_le(uint16_t, b, 0) | \
    shift_le(uint16_t, b, 1) )

  #define read_uint32_t(b) ( \
    shift_le(uint32_t, b, 0) | \
    shift_le(uint32_t, b, 1) | \
    shift_le(uint32_t, b, 2) | \
    shift_le(uint32_t, b, 3) )

  #define read_uint64_t(b) ( \
    shift_le(uint64_t, b, 0) | \
    shift_le(uint64_t, b, 1) | \
    shift_le(uint64_t, b, 2) | \
    shift_le(uint64_t, b, 3) | \
    shift_le(uint64_t, b, 4) | \
    shift_le(uint64_t, b, 5) | \
    shift_le(uint64_t, b, 6) | \
    shift_le(uint64_t, b, 7) )

  #define read_int8_t(b) (((union word_8){.u = read_uint8_t(b)}).i)
  #define read_int16_t(b) (((union word_16){.u = read_uint16_t(b)}).i)
  #define read_int32_t(b) (((union word_32){.u = read_uint32_t(b)}).i)
  #define read_int64_t(b) (((union word_64){.u = read_uint64_t(b)}).i)

  #define read_float(b) (((union word_32){.u = read_uint32_t(b)}).f)
  #define read_double(b) (((union word_64){.u = read_uint64_t(b)}).f)

#endif

#endif