#ifndef _GITISSUES_GLOBAL_H_
#define _GITISSUES_GLOBAL_H_

#include <gitissues/allocator.h>

struct GlobalContext {
  struct BlockAllocator lifetime;
  struct ImplicitAllocator transient;
};

void *lifetimeAllocate(uint32_t size, uint32_t alignment);
void *transientAllocate(uint32_t size, uint32_t alignment);
// Size of allocation
void freeTransient(void *ptr, uint32_t size);

struct ImplicitAllocator *getGlobalTransientAllocator(void);
struct BlockAllocator *getGlobalLifetimeAllocator(void);

void createGlobalContext(void);
void freeGlobalContext(void);

#endif
