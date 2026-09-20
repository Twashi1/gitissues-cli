#include "gitissues/allocator.h"
#include "gitissues/tests/test.h"
#include <gitissues/tests/allocator.h>
#include <string.h>

void testAllocator(void) {
  struct AllocatorContext ctx = {0};
  ctx.suite = createSuite("Allocator test suite");
  ctx.lifetime = createBlockAllocator(4096);
  ctx.transient = createImplicitAllocator();

  pushHeader(&ctx.suite, "Transient allocations");

  {
    pushTest(&ctx.suite, "Allocating blocks of <= 256");

    void *a = allocateImplicitAllocator(&ctx.transient, 255, 1);
    void *b = allocateImplicitAllocator(&ctx.transient, 256, 1);
    void *c = allocateImplicitAllocator(&ctx.transient, 254, 1);
    void *d = allocateImplicitAllocator(&ctx.transient, 230, 1);

    testPassed(&ctx.suite, NULL);

    pushTest(&ctx.suite, "Writing to allocations");

    // Write some strings and read them
    strcpy(a, "Hello worlda");
    strcpy(b, "Hello worldb");
    strcpy(c, "Hello worldc");
    strcpy(d, "Hello worldd");

    testPassed(&ctx.suite, NULL);

    // TODO: TEST_FAIL_IF is bad, need to re-think testing API
    pushTest(&ctx.suite, "Readback of allocations");

    TEST_FAIL_IF(&ctx.suite, strcmp(a, "Hello worlda") != 0);
    TEST_FAIL_IF(&ctx.suite, strcmp(b, "Hello worldb") != 0);
    TEST_FAIL_IF(&ctx.suite, strcmp(c, "Hello worldc") != 0);
    TEST_FAIL_IF(&ctx.suite, strcmp(d, "Hello worldd") != 0);
    testPassed(&ctx.suite, "All allocations passed");

    pushTest(&ctx.suite, "Freeing allocations");

    freeFastAllocationImplicitAllocator(&ctx.transient, a, 256);
    freeFastAllocationImplicitAllocator(&ctx.transient, b, 256);
    freeFastAllocationImplicitAllocator(&ctx.transient, c, 256);
    freeFastAllocationImplicitAllocator(&ctx.transient, d, 256);

    testPassed(&ctx.suite, NULL);
  }

  popHeader(&ctx.suite);

  freeSuite(&ctx.suite);
  freeBlockAllocator(&ctx.lifetime);
  freeImplicitAllocator(&ctx.transient);
}
