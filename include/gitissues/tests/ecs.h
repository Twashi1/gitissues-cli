#ifndef _GITISSUES_TESTS_ECS_H_
#define _GITISSUES_TESTS_ECS_H_

#include <gitissues/ecs/registry.h>
#include <gitissues/tests/test.h>

struct ECSContext {
  struct Suite suite;
  struct Registry registry;
  struct BlockAllocator allocator;
  struct UmbraString fizz;
  struct UmbraString buzz;
  Entity *entities;
  uint32_t entityArraySize;
};

void testECS(void);

#endif
