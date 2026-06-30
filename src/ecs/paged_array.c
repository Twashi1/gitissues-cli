#include <gitissues/defines.h>
#include <gitissues/ecs/paged_array.h>
#include <gitissues/errs.h>
#include <stdio.h>

struct SparseArray createSparseArray(void) {
    struct SparseArray array = {0};
    return array;
}

enum ErrorCode reserveIndexSparseArray(struct SparseArray* array, uint32_t index) {
    uint32_t page = _ECS_GET_PAGE(index);
    uint32_t newSize = page + 1;  // No growth factor, we expect to rellocate infrequently

    printf("Reserving page index %d, for index %d, new size: %d\n", page, index, newSize);

    if (LIKELY(array->size >= newSize))
        return GITISSUES_OK;

    Entity** p = (Entity**)realloc(array->pages, newSize * sizeof(Entity*));

    if (DEBUG_CONDITION(p == NULL)) {
        return GITISSUES_OUT_OF_MEMORY;
    }

    array->pages = p;
    array->size = newSize;
    // Allocate the page
    Entity* e = (Entity*)malloc(_ECS_PAGE_SIZE * sizeof(Entity));

    if (DEBUG_CONDITION(e == NULL)) {
        return GITISSUES_OUT_OF_MEMORY;
    }

    memset(e, 0xff, _ECS_PAGE_SIZE * sizeof(Entity));

    array->pages[page] = e;

    return GITISSUES_OK;
}

bool containsEntitySparseArray(struct SparseArray const* array, Entity entity) {
    uint32_t index = entityToPos(entity);
    uint32_t page = _ECS_GET_PAGE(index);
    uint32_t indexWithinPage = _ECS_INDEX_IN_PAGE(index);

    if (page < array->size) {
        printf("ContainsEntity: %d; for entity %d, index %d, page %d, index in page %d\n",
               array->pages[page][indexWithinPage], entity, index, page, indexWithinPage);
    }

    return (page < array->size) &&
           (((_ECS_TOMBSTONE & entity) ^ array->pages[page][indexWithinPage]) < _ECS_NULL);
}

uint32_t getElemEntitySparseArray(struct SparseArray const* array, Entity entity) {
    if (DEBUG_CONDITION(!containsEntitySparseArray(array, entity))) {
        return UINT32_MAX;
    }

    uint32_t index = entityToPos(entity);
    uint32_t pageIndex = _ECS_GET_PAGE(index);
    uint32_t indexWithinPage = _ECS_INDEX_IN_PAGE(index);

    return entityToPos(array->pages[pageIndex][indexWithinPage]);
}

enum ErrorCode addEntitySparseArray(struct SparseArray* array, Entity entity, uint32_t value) {
    printf("Adding entity to sparse array\n");

    if (DEBUG_CONDITION(containsEntitySparseArray(array, entity))) {
        printf("Contained entity already!\n");
        return GITISSUES_ENTITY_ALREADY_EXISTED;
    }

    uint32_t index = entityToPos(entity);

    printf("Reserving index %d in sparse array\n", index);

    enum ErrorCode ec = reserveIndexSparseArray(array, index);
    if (ec != GITISSUES_OK) {
        return ec;
    }

    uint32_t pageIndex = _ECS_GET_PAGE(index);
    uint32_t indexWithinPage = _ECS_INDEX_IN_PAGE(index);

    array->pages[pageIndex][indexWithinPage] = value;

    return GITISSUES_OK;
}

enum ErrorCode releaseEntitySparseArray(struct SparseArray* array, Entity entity) {
    if (DEBUG_CONDITION(!containsEntitySparseArray(array, entity))) {
        return GITISSUES_ENTITY_DID_NOT_EXIST;
    }

    uint32_t index = entityToPos(entity);
    uint32_t pageIndex = _ECS_GET_PAGE(index);
    uint32_t indexWithinPage = _ECS_INDEX_IN_PAGE(index);

    // TODO: maybe more correct is just _ECS_NULL
    array->pages[pageIndex][indexWithinPage] = _ECS_NULL | _ECS_DEAD;

    return GITISSUES_OK;
}

void freeSparseArray(struct SparseArray* array) {
    for (uint32_t i = 0; i < array->size; i++) {
        free(array->pages[i]);
    }

    free(array->pages);
}
