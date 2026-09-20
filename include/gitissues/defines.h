#ifndef _GITISSUES_DEFINES_H_
#define _GITISSUES_DEFINES_H_

#include <assert.h>
#include <stdlib.h>

#if defined(__clang__) || defined(__GNUC__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

// TODO: for testing behaviour, we'd like to test invalid inputs are flagged (at
// least in debug mode) so override DEBUG_ASSERT
#ifdef NDEBUG
#define DEBUG_STATEMENT(x) ((void)0)
// Assuming that the condition has no side-effects
#define DEBUG_CONDITION(x) (false && (x))
#define DEBUG_ASSERT(x, msg) ((void)0)
#define NDEBUG_STATEMENT(x) x
#else
#define NDEBUG_STATEMENT(x) ((void)0)
#define DEBUG_STATEMENT(x)                                                     \
  do {                                                                         \
    x                                                                          \
  } while (0)
#define DEBUG_CONDITION(x) (x)
#define DEBUG_ASSERT(x, msg) assert((x) && (msg));
#endif

#define CARRAY_SIZE(array) (sizeof(array) / sizeof(*(array)))
#define ARRAY_GROWTH_ONE_HALF(cap) (cap + (cap >> 1) + 1)
#define ARRAY_GROWTH_PLUS_ONE(cap) (cap + 1)
#define ARRAY_RESERVE(array, newCap, growth)                                   \
  do {                                                                         \
    if ((array).capacity < (newCap)) {                                         \
      (array).capacity = growth((array).capacity) > newCap                     \
                             ? growth((array).capacity)                        \
                             : newCap;                                         \
      (array).data =                                                           \
          realloc((array).data, (array).capacity * sizeof(*(array).data));     \
      DEBUG_ASSERT((array).data != NULL,                                       \
                   "Out of memory reserving space for array");                 \
    }                                                                          \
  } while (0)
#define ARRAY_APPEND(array, item, growth)                                      \
  do {                                                                         \
    ARRAY_RESERVE((array), (array).size + 1, growth);                          \
    (array).data[(array).size++] = (item);                                     \
  } while (0)
#define ARRAY_REMOVE(array, index)                                             \
  do {                                                                         \
    DEBUG_ASSERT((array).size > (index),                                       \
                 "Removing from index larger than array");                     \
    if ((array).size > (index)) {                                              \
      (array).data[(index)] = (array).data[(array).size - 1];                  \
    }                                                                          \
    (array).size--;                                                            \
  } while (0)
#define NDEBUG_ASSERT(x, ...)                                                  \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "[FATAL]: ");                                            \
      fprintf(stderr, __VA_ARGS__);                                            \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#endif
