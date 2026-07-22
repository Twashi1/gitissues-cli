#include <gitissues/defines.h>
#include <gitissues/tests/allocator.h>
#include <gitissues/tests/ecs.h>
#include <gitissues/tests/json.h>
#include <gitissues/tests/test.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Suite createSuite(char const *name) {
  struct Suite suite = {0};
  suite.name = name;

  printf("Suite %s\n", name);

  pushHeader(&suite, "Full suite");

  return suite;
}

void freeSuite(struct Suite *suite) {
  DEBUG_ASSERT(suite->headers.size == 1,
               "Only one header should be left, popped too much/too little");
  struct Header *header = &suite->headers.data[0];

  popHeader(suite);

  // Note reference still alive, we haven't freed the data yet
  if (header->numPassed == header->numTests) {
    printf("Suite %s: All tests passed!\n", suite->name);
  } else {
    printf("Suite %s: Some tests failed\n", suite->name);
  }

  free(suite->headers.data);
  free(suite->tests.data);
}

void pushTest(struct Suite *suite, char const *name) {
  if (suite->tests.size >= suite->tests.capacity) {
    uint32_t newCapacity = suite->tests.capacity * 2 + 1;

    void *p = realloc(suite->tests.data, newCapacity * sizeof(char const *));
    DEBUG_ASSERT(p != NULL, "Failed to reallocate suite tests array");
    suite->tests.data = (char const **)p;
    suite->tests.capacity = newCapacity;
  }

  suite->tests.data[suite->tests.size++] = name;
}

void testFailed(struct Suite *suite, char const *reason) {
  if (suite->tests.size == 0)
    return;

  // TODO: additional context like line number passed in through a macro
  char const *testName = suite->tests.data[suite->tests.size - 1];
  suite->tests.size--;

  int depth = suite->headers.size + 1;
  char buf[16] = {0};
  // Set limit so we don't overrun buffer
  depth = depth > 15 ? 15 : depth;
  memset(buf, (int)'\t', depth);

  if (reason != NULL) {
    printf("%s%s: %s\n", buf, testName, reason);
  } else {
    printf("%s%s\n", buf, testName);
  }

  for (uint32_t i = 0; i < suite->headers.size; i++) {
    suite->headers.data[i].numTests++;
  }
}

void testPassed(struct Suite *suite, char const *message) {
  if (suite->tests.size == 0)
    return;

  // TODO: additional context like line number passed in through a macro
  char const *testName = suite->tests.data[suite->tests.size - 1];
  suite->tests.size--;

  int depth = suite->headers.size + 1;
  char buf[16] = {0};
  // Set limit so we don't overrun buffer
  depth = depth > 15 ? 15 : depth;
  memset(buf, (int)'\t', depth);

  if (message != NULL) {
    printf("%s%s: %s\n", buf, testName, message);
  } else {
    printf("%s%s\n", buf, testName);
  }

  for (uint32_t i = 0; i < suite->headers.size; i++) {
    suite->headers.data[i].numTests++;
    suite->headers.data[i].numPassed++;
  }
}

void pushHeader(struct Suite *suite, char const *name) {
  if (suite->headers.size >= suite->headers.capacity) {
    uint32_t newCapacity = suite->headers.capacity * 2 + 1;

    void *p = realloc(suite->headers.data, newCapacity * sizeof(struct Header));
    DEBUG_ASSERT(p != NULL, "Failed to reallocate suite headers array");
    suite->headers.data = (struct Header *)p;
    suite->headers.capacity = newCapacity;
  }

  struct Header header = {0};
  header.name = name;

  suite->headers.data[suite->headers.size++] = header;

  int depth = suite->headers.size - 1;
  char buf[16] = {0};
  // Set limit so we don't overrun buffer
  depth = depth > 15 ? 15 : depth;
  memset(buf, (int)'\t', depth);

  printf("%s[%s]\n", buf, name);
}

void popHeader(struct Suite *suite) {
  struct Header const *header = &suite->headers.data[suite->headers.size - 1];

  int depth = suite->headers.size - 1;
  char buf[16] = {0};
  // Set limit so we don't overrun buffer
  depth = depth > 15 ? 15 : depth;
  memset(buf, (int)'\t', depth);

  float percent = (float)header->numPassed / (float)header->numTests * 100.0f;

  printf("%s[%s] Passed [%d/%d] %.2f%%\n", buf, header->name, header->numPassed,
         header->numTests, percent);

  --suite->headers.size;
}

int main(void) {
  testECS();
  testJson();
  testAllocator();

  return 0;
}
