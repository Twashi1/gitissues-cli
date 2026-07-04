#ifndef _GITISSUES_TESTS_JSON_H_
#define _GITISSUES_TESTS_JSON_H_

#include <gitissues/json/json.h>
#include <gitissues/tests/test.h>

struct JsonContext {
  struct Suite suite;
  struct BlockAllocator allocator;
};

void testJson(void);

#endif
