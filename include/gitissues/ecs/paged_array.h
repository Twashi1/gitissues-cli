#ifndef _GITISSUES_ECS_PAGED_ARRAY_H_
#define _GITISSUES_ECS_PAGED_ARRAY_H_

#include <gitissues/defines.h>
#include <gitissues/ecs/entity.h>
#include <gitissues/errs.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define _ECS_PAGE_SIZE ((uint32_t)1024)
#define _ECS_GET_PAGE(index) (index / _ECS_PAGE_SIZE)
#define _ECS_INDEX_IN_PAGE(index) (index & (_ECS_PAGE_SIZE - 1))

// TODO: should be pages of uint32_t, the sparse array maps from an Entity index -> index in dense
// TODO: additionally support the swap function
// TODO: only the lower 20 bits are used (as a mapping to the dense array), the approach
// describes a use case for the higher 12 bits by treating the element in the sparse array as an
// entity
// TODO: ideally we allow it to either by an entity/component id
struct SparseArray {
    Entity** pages;
    uint32_t size;
};

struct SparseArray createSparseArray(void);
enum ErrorCode reserveSparseArray(struct SparseArray* array);
enum ErrorCode reserveIndexSparseArray(struct SparseArray* array, uint32_t index);
bool containsEntitySparseArray(struct SparseArray const* array, Entity entity);
// Returns uint32_t: a mask of SparseElem only keeping the bottom 20 bits (we store versions in the
// top 12)
uint32_t getElemEntitySparseArray(struct SparseArray const* array, Entity entity);
// uint32_t* getEntityPtrSparseArray(struct SparseArray* array, Entity entity); -- implement if
// needed
enum ErrorCode addEntitySparseArray(struct SparseArray* array, Entity entity, uint32_t value);
enum ErrorCode releaseEntitySparseArray(struct SparseArray* array, Entity entity);
void swapRemoveEntitySparseArray(struct SparseArray* array, Entity remove, Entity keep);
void freeSparseArray(struct SparseArray* array);

#endif
