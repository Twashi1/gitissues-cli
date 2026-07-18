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
#else
#define DEBUG_STATEMENT(x)                                                     \
  do {                                                                         \
    x                                                                          \
  } while (0)
#define DEBUG_CONDITION(x) (x)
#define DEBUG_ASSERT(x, msg) assert((x) && msg);
#endif

#define ARRAY_GROWTH_FACTOR(cap) ((uint32_t)(cap + (cap >> 1) + 1))
#define ARRAY_APPEND(array, item)                                              \
  do {                                                                         \
    if (array.size >= array.capacity) {                                        \
      array.capacity = ARRAY_GROWTH_FACTOR(array.capacity);                    \
      array.data = realloc(array.data, array.capacity * sizeof(*array.data));  \
      DEBUG_ASSERT(array.data != NULL, "Out of memory in appending to array"); \
    }                                                                          \
    array.data[array.size++] = item;                                           \
  } while (0)

#endif
