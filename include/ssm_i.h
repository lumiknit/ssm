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


#endif