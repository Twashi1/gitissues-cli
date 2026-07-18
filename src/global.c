#include <gitissues/global.h>

static struct GlobalContext global = {0};

void *lifetimeAllocate(uint32_t size, uint32_t alignment) {
  return allocateBlockAllocator(&global.lifetime, size, alignment);
}

void *transientAllocate(uint32_t size, uint32_t alignment) {
  return allocateImplicitAllocator(&global.transient, size, alignment);
}

// Size of allocation
void freeTransient(void *ptr, uint32_t size) {
  // TODO: bad practice; malloc probably faster
  if (size == 0) {
    freeAllocationImplicitAllocator(&global.transient, ptr);
    return;
  }

  freeFastAllocationImplicitAllocator(&global.transient, ptr, size);
}

void createGlobalContext(void) {
  global.lifetime = createBlockAllocator(4096);
  global.transient = createImplicitAllocator();
}

void freeGlobalContext(void) {
  freeBlockAllocator(&global.lifetime);
  freeImplicitAllocator(&global.transient);
}

struct ImplicitAllocator *getGlobalTransientAllocator(void) {
  return &global.transient;
}
struct BlockAllocator *getGlobalLifetimeAllocator(void) {
  return &global.lifetime;
}
