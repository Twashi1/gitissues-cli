#ifndef _GITISSUES_TESTS_TEST_H_
#define _GITISSUES_TESTS_TEST_H_

#include <stdint.h>

struct Header {
  char const *name;
  int numPassed;
  int numTests;
};

struct Suite {
  char const *name;

  struct {
    struct Header *data;
    uint32_t size;
    uint32_t capacity;
  } headers;

  struct {
    char const **data;
    uint32_t size;
    uint32_t capacity;
  } tests;
};

// instead of passing function
struct Suite createSuite(char const *name);
void freeSuite(struct Suite *suite);
void pushTest(struct Suite *suite, char const *name);
void testFailed(struct Suite *suite, char const *reason);
void testPassed(struct Suite *suite, char const *message);

void pushHeader(struct Suite *suite, char const *name);
void popHeader(struct Suite *suite);

#define TEST_PASS_CONDITION(suite, condition)                                  \
  do {                                                                         \
    if (!!(condition)) {                                                       \
      testPassed((suite), #condition);                                         \
    } else {                                                                   \
      testFailed((suite), #condition);                                         \
    }                                                                          \
  } while (0)

#define TEST_FAIL_IF(suite, condition)                                         \
  do {                                                                         \
    if (!!(condition)) {                                                       \
      testFailed((suite), #condition);                                         \
    }                                                                          \
  } while (0)

#endif
