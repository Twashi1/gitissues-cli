#ifndef _GITISSUES_ALLOCATOR_H_
#define _GITISSUES_ALLOCATOR_H_

#include <gitissues/defines.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define _GITISSUES_DEFAULT_ARENA_COUNT 4

struct Arena {
  void *data;
  uint32_t offset;
  uint32_t capacity;
};

struct BlockAllocator {
  struct Arena *arenas;
  uint32_t numArenas;
  uint32_t capacityArenas;
  uint32_t blockSize;
};

struct UniformSizeImplicitBuffer {
  uint8_t *data;
  uint32_t offset;
  uint32_t capacity;
  uintptr_t next;
  struct UniformSizeImplicitBuffer *nextObject;
};

// Implicit lists within each uniform size implicit buffer
struct ImplicitAllocator {
  _Static_assert(sizeof(uintptr_t) <= 8,
                 "Smallest byte buffer must be large enough for an individual "
                 "element to store a pointer address");
  struct UniformSizeImplicitBuffer buffer8;
  struct UniformSizeImplicitBuffer buffer16;
  struct UniformSizeImplicitBuffer buffer24;
  struct UniformSizeImplicitBuffer buffer32;
  struct UniformSizeImplicitBuffer buffer48;
  struct UniformSizeImplicitBuffer buffer64;
  struct UniformSizeImplicitBuffer buffer256;
  // Larger objects get malloc'd

  // TODO: keeping our own storage instead of an extra malloc
  // Additional buffers for when we fill one of the buffers above
  // struct {
  //   struct GroupBuffer data;
  //   uint32_t size;
  //   uint32_t capacity;
  // } additionalBuffers;
};

struct UniformSizeImplicitBuffer createImplicitBuffer(uint32_t blockSize);
void freeImplicitBuffer(struct UniformSizeImplicitBuffer *buffer);
bool freeAllocationImplicitBuffer(struct UniformSizeImplicitBuffer *buffer,
                                  void *data);
void *allocateImplicitBuffer(struct UniformSizeImplicitBuffer *groupBuffer,
                             uint32_t allocationSize, uint32_t alignment);

struct ImplicitAllocator createImplicitAllocator(void);
void freeImplicitAllocator(struct ImplicitAllocator *allocator);
void freeAllocationImplicitAllocator(struct ImplicitAllocator *allocator,
                                     void *data);
// We can speed-up cleanup if we know the size of the allocation
void freeFastAllocationImplicitAllocator(struct ImplicitAllocator *allocator,
                                         void *data, uint32_t objectSize);
void *allocateImplicitAllocator(struct ImplicitAllocator *allocator,
                                uint32_t allocationSize, uint32_t alignment);

uint32_t alignUp(uint32_t n, uint32_t align);
struct Arena createArena(uint32_t size);
void freeArena(struct Arena *arena);
void *allocateArena(struct Arena *arena, uint32_t allocationSize,
                    uint32_t alignment);

// TODO: should be declared as static?
bool _fitBlockAllocator(struct BlockAllocator *allocator, uint32_t numArenas);

struct BlockAllocator createBlockAllocator(uint32_t blockSize);
void freeBlockAllocator(struct BlockAllocator *allocator);
void *allocateBlockAllocator(struct BlockAllocator *allocator,
                             uint32_t allocationSize, uint32_t alignment);

#endif
