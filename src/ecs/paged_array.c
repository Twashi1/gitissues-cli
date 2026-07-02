#include <gitissues/defines.h>
#include <gitissues/ecs/paged_array.h>
#include <gitissues/errs.h>
#include <gitissues/log.h>

struct SparseArray createSparseArray(void) {
    struct SparseArray array = {0};
    return array;
}

enum ErrorCode reserveIndexSparseArray(struct SparseArray* array, uint32_t index) {
    uint32_t page = _ECS_GET_PAGE(index);
    uint32_t newSize = page + 1;  // No growth factor, we expect to rellocate infrequently

    GITISSUES_LOG_DEBUG("Reserving page index %d, for index %d, new size: %d\n", page, index,
                        newSize);

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
    if (DEBUG_CONDITION(containsEntitySparseArray(array, entity))) {
        return GITISSUES_ENTITY_ALREADY_EXISTED;
    }

    uint32_t index = entityToPos(entity);

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

void saveSparseArray(struct SparseArray const* array, FILE* p) {
    // struct SparseArray {
    //     Entity** pages;
    //     uint32_t size;
    // };
    // Write page size
    uint32_t pageSize = _ECS_PAGE_SIZE;
    fwrite(&pageSize, sizeof(pageSize), 1, p);
    // Write number of pages
    fwrite(&array->size, sizeof(array->size), 1, p);
    // Iterate pages and write
    for (uint32_t i = 0; i < array->size; i++) {
        Entity* page = array->pages[i];
        DEBUG_ASSERT(page != NULL, "Null page when saving sparse array");

        fwrite(page, sizeof(Entity), _ECS_PAGE_SIZE, p);
    }
}

struct SparseArray loadSparseArray(FILE* p) {
    struct SparseArray array;

    uint32_t pageSize = 0;
    fread(&pageSize, sizeof(pageSize), 1, p);
    DEBUG_ASSERT(pageSize == _ECS_PAGE_SIZE, "Loaded page size was different to definition");
    fread(&array.size, sizeof(array.size), 1, p);

    enum ErrorCode ec = reserveIndexSparseArray(&array, (pageSize - 1) * _ECS_PAGE_SIZE);
    DEBUG_PRINT_ERROR("Failed to reserve sparse array after load: %s\n", ec);
    DEBUG_ASSERT(ec == GITISSUES_OK, "Reserve index sparse array failed");

    for (uint32_t i = 0; i < array.size; i++) {
        Entity* page = array.pages[i];
        fread(page, sizeof(Entity), _ECS_PAGE_SIZE, p);
    }

    return array;
}
