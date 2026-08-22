#ifndef _GITISSUES_TESTS_IFF_H_
#define _GITISSUES_TESTS_IFF_H_

#include <gitissues/iff/iff.h>
#include <gitissues/tests/test.h>

struct IFFContext {
  struct Suite suite;
  struct BlockAllocator allocator;
  struct Schema schema;
};

void testIFF(void);

#endif
