#ifndef _GITISSUES_TESTS_ALLOCATOR_H_
#define _GITISSUES_TESTS_ALLOCATOR_H_

#include <gitissues/allocator.h>
#include <gitissues/tests/test.h>

struct AllocatorContext {
  struct Suite suite;
  struct BlockAllocator lifetime;
  struct ImplicitAllocator transient;
};

void testAllocator(void);

#endif
