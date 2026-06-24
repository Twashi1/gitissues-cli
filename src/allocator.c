#include <alloca.h>
#include <gitissues/allocator.h>

uint32_t alignUp(uint32_t n, uint32_t align) {
    return (n + align - 1) & ~(align - 1);
}

struct Arena createArena(uint32_t size) {
    struct Arena arena;
    arena.offset = 0;
    arena.capacity = size;
    arena.data = malloc(size);

    return arena;
}

void dropArena(struct Arena* arena) {
    free(arena->data);
}

void* allocateArena(struct Arena* arena, uint32_t allocationSize, uint32_t alignment) {
    uint32_t start = alignUp(arena->offset, alignment);
    uint32_t end = start + allocationSize;

    if (end > arena->capacity) {
        return NULL;
    }

    void* p = (uint8_t*)arena->data + start;
    arena->offset = end;

    return p;
}

struct BlockAllocator createBlockAllocator(uint32_t blockSize) {
    struct BlockAllocator allocator;
    allocator.arenas = NULL;
    allocator.capacityArenas = 0;
    allocator.blockSize = blockSize;
    allocator.numArenas = 0;

    _fitBlockAllocator(&allocator, _GITISSUES_DEFAULT_ARENA_COUNT);

    return allocator;
}

void dropBlockAllocator(struct BlockAllocator* allocator) {
    for (uint32_t i = 0; i < allocator->numArenas; i++) {
        dropArena(&allocator->arenas[i]);
    }

    free(allocator->arenas);
}

bool _fitBlockAllocator(struct BlockAllocator* allocator, uint32_t numArenas) {
    if (numArenas < allocator->capacityArenas) {
        return true;
    }

    uint32_t growSize = allocator->capacityArenas + (allocator->capacityArenas >> 1);
    uint32_t newSize = numArenas > growSize ? numArenas : growSize;

    if (allocator->arenas == NULL) {
        allocator->arenas = malloc(sizeof(struct Arena) * newSize);
    } else {
        void* tmp = realloc(allocator->arenas, sizeof(struct Arena) * newSize);

        allocator->arenas = tmp;
    }

    allocator->capacityArenas = newSize;

    if (allocator->arenas == NULL) {
        return false;
    }

    return true;
}

void* allocateBlock(struct BlockAllocator* allocator, uint32_t allocationSize, uint32_t alignment) {
    // Go to front block
    struct Arena* front = &allocator->arenas[allocator->numArenas - 1];
    void* p = allocateArena(front, allocationSize, alignment);

    if (p != NULL) {
        return p;
    }

    // Ensure enough space in internal dynamic array to fit new arena
    if (UNLIKELY(!_fitBlockAllocator(allocator, allocator->numArenas + 1))) {
        return NULL;
    }

    // Create arena with at least enough size to fit this allocation
    uint64_t arenaBlockSize =
        allocationSize < allocator->blockSize ? allocator->blockSize : allocationSize;

    struct Arena* newArena = &allocator->arenas[allocator->numArenas++];
    *newArena = createArena(arenaBlockSize);

    return allocateArena(newArena, allocationSize, alignment);
}
