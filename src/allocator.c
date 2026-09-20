#include "gitissues/defines.h"
#include <alloca.h>
#include <gitissues/allocator.h>
#include <gitissues/log.h>

struct UniformSizeImplicitBuffer createImplicitBuffer(uint32_t blockSize) {
  struct UniformSizeImplicitBuffer buffer = {0};
  buffer.nextObject = NULL;
  buffer.next = (uintptr_t)NULL;
  // buffer.elementSize = elementSize;

  buffer.data = malloc(blockSize);
  DEBUG_ASSERT(buffer.data != NULL,
               "Failed to allocate block for group buffer");
  buffer.capacity = blockSize;

  return buffer;
}

void freeImplicitBuffer(struct UniformSizeImplicitBuffer *buffer) {
  struct UniformSizeImplicitBuffer *current = buffer;

  while (current != NULL) {
    free(current->data);
    struct UniformSizeImplicitBuffer *group = current;
    current = current->nextObject;
    free(group);
  };
}

bool freeAllocationImplicitBuffer(struct UniformSizeImplicitBuffer *buffer,
                                  void *data) {
  if (data == NULL)
    return true;
  // start at current
  // compare base pointer of buffer
  // if within range, clear that allocation

  // bounds to check
  // buffer.data, buffer.data + buffer.capacity
  // check data < buffer.data, and data > buffer.data + buffer.capacity
  uintptr_t dataAddr = (uintptr_t)data;

  for (struct UniformSizeImplicitBuffer *current = buffer; current != NULL;
       current = current->nextObject) {
    uintptr_t left = (uintptr_t)current->data;
    uintptr_t right = (uintptr_t)(current->data + current->capacity);

    // Out of bounds, must not be inside this allocation
    if (dataAddr < left || dataAddr >= right) {
      continue;
    }

    // calculate the offset
    uintptr_t allocationOffset = dataAddr - left;

    // now we increment the implicit list
    // slot allocationOffset is now free; so we set the value of
    // index @ allocationOffset to next, and replace next with allocationOffset
    *(uintptr_t *)(dataAddr) = current->next;
    current->next = allocationOffset;

    return true;
  }

  // Couldn't find allocation
  return false;
}

void *allocateImplicitBuffer(struct UniformSizeImplicitBuffer *buffer,
                             uint32_t allocationSize, uint32_t alignment) {
  DEBUG_ASSERT(
      alignment <= allocationSize,
      "Our alignment requirement is always less or equally as stringent as the "
      "size of the allocation; allows us to skip alignment checks");
  DEBUG_ASSERT(buffer != NULL, "Expected non-null allocator");

  for (struct UniformSizeImplicitBuffer *current = buffer; current != NULL;
       current = current->nextObject) {
    // Check if we have an existing slot available
    if (current->next != (uintptr_t)NULL) {
      // Look at the address of next, and set that to new next
      uintptr_t availableSlot = current->next;
      current->next = *(uintptr_t *)current->next;

      return (void *)availableSlot;
    }

    // Use a new slot
    uint32_t start = current->offset;
    uint32_t end = current->offset + allocationSize;
    DEBUG_ASSERT(alignUp(current->offset, alignment) == start,
                 "Alignment requirement created a gap");

    // Make sure we check the other nextObjects
    if (end >= current->capacity) {
      continue;
    }

    void *p = current->data + start;
    current->offset = end;

    return p;
  }

  // We just malloc new blocks
  struct UniformSizeImplicitBuffer *newBuffer =
      malloc(sizeof(struct UniformSizeImplicitBuffer));
  DEBUG_ASSERT(newBuffer != NULL, "Out of memory allocating GroupBuffer");

  // We swap the pointers; we want the fresher allocation to be searched first
  *newBuffer = *buffer;
  // Create a new buffer with the same block size;
  *buffer = createImplicitBuffer(buffer->capacity);
  // Point to next
  buffer->nextObject = newBuffer;

  // Now allocate on ourselves, we could do this through a recursive call, but
  // for sanity/optimisation, we'll just write out allocating on index 0
  void *p = buffer->data;
  DEBUG_ASSERT(buffer->offset == 0 && buffer->capacity >= allocationSize,
               "Newly allocated buffer had invalid starting offset/capacity");
  buffer->offset += allocationSize;

  return p;
}

struct ImplicitAllocator createImplicitAllocator(void) {
  struct ImplicitAllocator allocator;
  allocator.buffer8 = createImplicitBuffer(8 * 1024);
  allocator.buffer16 = createImplicitBuffer(16 * 512);
  allocator.buffer24 = createImplicitBuffer(24 * 512);
  allocator.buffer32 = createImplicitBuffer(32 * 512);
  allocator.buffer48 = createImplicitBuffer(48 * 256);
  allocator.buffer64 = createImplicitBuffer(64 * 256);
  allocator.buffer256 = createImplicitBuffer(256 * 128);

  return allocator;
}

void freeImplicitAllocator(struct ImplicitAllocator *allocator) {
  if (allocator == NULL)
    return;

  freeImplicitBuffer(allocator->buffer8.nextObject);
  freeImplicitBuffer(allocator->buffer16.nextObject);
  freeImplicitBuffer(allocator->buffer24.nextObject);
  freeImplicitBuffer(allocator->buffer32.nextObject);
  freeImplicitBuffer(allocator->buffer48.nextObject);
  freeImplicitBuffer(allocator->buffer64.nextObject);
  freeImplicitBuffer(allocator->buffer256.nextObject);

  free(allocator->buffer8.data);
  free(allocator->buffer16.data);
  free(allocator->buffer24.data);
  free(allocator->buffer32.data);
  free(allocator->buffer48.data);
  free(allocator->buffer64.data);
  free(allocator->buffer256.data);
}

void freeAllocationImplicitAllocator(struct ImplicitAllocator *allocator,
                                     void *data) {
  if (freeAllocationImplicitBuffer(&allocator->buffer8, data))
    return;
  if (freeAllocationImplicitBuffer(&allocator->buffer16, data))
    return;
  if (freeAllocationImplicitBuffer(&allocator->buffer24, data))
    return;
  if (freeAllocationImplicitBuffer(&allocator->buffer32, data))
    return;
  if (freeAllocationImplicitBuffer(&allocator->buffer48, data))
    return;
  if (freeAllocationImplicitBuffer(&allocator->buffer64, data))
    return;
  if (freeAllocationImplicitBuffer(&allocator->buffer256, data))
    return;

  // we must've defaulted to malloc
  free(data);
}

void freeFastAllocationImplicitAllocator(struct ImplicitAllocator *allocator,
                                         void *data, uint32_t objectSize) {
  if (objectSize <= 8) {
    DEBUG_ASSERT(freeAllocationImplicitBuffer(&allocator->buffer8, data),
                 "Failed to free object");
    return;
  }
  if (objectSize <= 16) {
    DEBUG_ASSERT(freeAllocationImplicitBuffer(&allocator->buffer16, data),
                 "Failed to free object");
    return;
  }
  if (objectSize <= 24) {
    DEBUG_ASSERT(freeAllocationImplicitBuffer(&allocator->buffer24, data),
                 "Failed to free object");
    return;
  }
  if (objectSize <= 32) {
    DEBUG_ASSERT(freeAllocationImplicitBuffer(&allocator->buffer32, data),
                 "Failed to free object");
    return;
  }
  if (objectSize <= 48) {
    DEBUG_ASSERT(freeAllocationImplicitBuffer(&allocator->buffer48, data),
                 "Failed to free object");
    return;
  }
  if (objectSize <= 64) {
    DEBUG_ASSERT(freeAllocationImplicitBuffer(&allocator->buffer64, data),
                 "Failed to free object");
    return;
  }
  if (objectSize <= 256) {
    DEBUG_ASSERT(freeAllocationImplicitBuffer(&allocator->buffer256, data),
                 "Failed to free object");
    return;
  }

  GITISSUES_LOG_DEBUG("Defaulting to free in freeTransientAllocation");
  free(data);
}

void *allocateImplicitAllocator(struct ImplicitAllocator *allocator,
                                uint32_t allocationSize, uint32_t alignment) {
  if (allocationSize <= 8) {
    return allocateImplicitBuffer(&allocator->buffer8, 8, alignment);
  }
  if (allocationSize <= 16) {
    return allocateImplicitBuffer(&allocator->buffer16, 16, alignment);
  }
  if (allocationSize <= 24) {
    return allocateImplicitBuffer(&allocator->buffer24, 24, alignment);
  }
  if (allocationSize <= 32) {
    return allocateImplicitBuffer(&allocator->buffer32, 32, alignment);
  }
  if (allocationSize <= 48) {
    return allocateImplicitBuffer(&allocator->buffer48, 48, alignment);
  }
  if (allocationSize <= 64) {
    return allocateImplicitBuffer(&allocator->buffer64, 64, alignment);
  }
  if (allocationSize <= 256) {
    return allocateImplicitBuffer(&allocator->buffer256, 256, alignment);
  }

  return malloc(allocationSize);
}

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

void freeArena(struct Arena *arena) { free(arena->data); }

void *allocateArena(struct Arena *arena, uint32_t allocationSize,
                    uint32_t alignment) {
  uint32_t start = alignUp(arena->offset, alignment);
  uint32_t end = start + allocationSize;

  if (end > arena->capacity) {
    return NULL;
  }

  void *p = (uint8_t *)arena->data + start;
  arena->offset = end;

  return p;
}

struct BlockAllocator createBlockAllocator(uint32_t blockSize) {
  struct BlockAllocator allocator;
  allocator.arenas.data = NULL;
  allocator.arenas.size = 0;
  allocator.arenas.capacity = 0;
  allocator.blockSize = blockSize;

  ARRAY_RESERVE(allocator.arenas, _GITISSUES_DEFAULT_ARENA_COUNT,
                ARRAY_GROWTH_PLUS_ONE);

  return allocator;
}

void freeBlockAllocator(struct BlockAllocator *allocator) {
  for (uint32_t i = 0; i < allocator->arenas.size; i++) {
    freeArena(&allocator->arenas.data[i]);
  }

  free(allocator->arenas.data);
}

void *allocateBlockAllocator(struct BlockAllocator *allocator,
                             uint32_t allocationSize, uint32_t alignment) {
  // If we have an arena allocated
  if (allocator->arenas.size > 0) {
    // Go to the last block
    // TODO: iterate arenas to find other possible slots?
    struct Arena *front = &allocator->arenas.data[allocator->arenas.size - 1];
    void *p = allocateArena(front, allocationSize, alignment);

    if (p != NULL) {
      return p;
    }
  }

  // Create arena with at least enough size to fit this allocation
  uint64_t arenaBlockSize = allocationSize < allocator->blockSize
                                ? allocator->blockSize
                                : allocationSize;
  struct Arena newArena = createArena(arenaBlockSize);
  void *p = allocateArena(&newArena, allocationSize, alignment);
  DEBUG_ASSERT(p != NULL, "Failed to allocate space in fresh arena?");

  // Ensure enough space in internal dynamic array to fit new arena
  ARRAY_APPEND(allocator->arenas, newArena, ARRAY_GROWTH_PLUS_ONE);

  return p;
}
