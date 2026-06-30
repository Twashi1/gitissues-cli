#ifndef _GITISSUES_ALLOCATOR_H_
#define _GITISSUES_ALLOCATOR_H_

#include <gitissues/defines.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define _GITISSUES_DEFAULT_ARENA_COUNT 4

struct Arena {
    void* data;
    uint32_t offset;
    uint32_t capacity;
};

struct BlockAllocator {
    struct Arena* arenas;
    uint32_t numArenas;
    uint32_t capacityArenas;
    uint32_t blockSize;
};

uint32_t alignUp(uint32_t n, uint32_t align);
struct Arena createArena(uint32_t size);
void dropArena(struct Arena* arena);
void* allocateArena(struct Arena* arena, uint32_t allocationSize, uint32_t alignment);

// TODO: should be declared as static?
bool _fitBlockAllocator(struct BlockAllocator* allocator, uint32_t numArenas);

struct BlockAllocator createBlockAllocator(uint32_t blockSize);
void dropBlockAllocator(struct BlockAllocator* allocator);
void* allocateBlockAllocator(struct BlockAllocator* allocator, uint32_t allocationSize,
                             uint32_t alignment);

#endif
