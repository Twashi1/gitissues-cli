#include <gitissues/defines.h>
#include <gitissues/ecs/paged_array.h>
#include <gitissues/log.h>

struct SparseArray createSparseArray(void) {
  struct SparseArray array = {0};
  return array;
}

void reserveIndexSparseArray(struct SparseArray *array, uint32_t index) {
  uint32_t page = _ECS_GET_PAGE(index);
  uint32_t newSize =
      page + 1; // No growth factor, we expect to rellocate infrequently

  GITISSUES_LOG_DEBUG("Reserving page index %d, for index %d, new size: %d",
                      page, index, newSize);

  if (LIKELY(array->size >= newSize))
    return;

  Entity **p = (Entity **)realloc(array->pages, newSize * sizeof(Entity *));
  DEBUG_ASSERT(p != NULL, "Failed to reallocate pages in sparse array");

  array->pages = p;
  array->size = newSize;
  // Allocate the page
  Entity *e = (Entity *)malloc(_ECS_PAGE_SIZE * sizeof(Entity));
  DEBUG_ASSERT(e != NULL,
               "Failed to allocate space for a page in sparse array");

  memset(e, 0xff, _ECS_PAGE_SIZE * sizeof(Entity));

  array->pages[page] = e;
}

bool containsEntitySparseArray(struct SparseArray const *array, Entity entity) {
  uint32_t index = entityToPos(entity);
  uint32_t page = _ECS_GET_PAGE(index);
  uint32_t indexWithinPage = _ECS_INDEX_IN_PAGE(index);

  return (page < array->size) &&
         (((_ECS_TOMBSTONE & entity) ^ array->pages[page][indexWithinPage]) <
          _ECS_NULL);
}

uint32_t getIndexSparseArray(struct SparseArray const *array, Entity entity) {
  if (DEBUG_CONDITION(!containsEntitySparseArray(array, entity))) {
    return UINT32_MAX;
  }

  uint32_t index = entityToPos(entity);
  uint32_t pageIndex = _ECS_GET_PAGE(index);
  uint32_t indexWithinPage = _ECS_INDEX_IN_PAGE(index);

  return entityToPos(array->pages[pageIndex][indexWithinPage]);
}

void addEntitySparseArray(struct SparseArray *array, Entity entity,
                          uint32_t value) {
  DEBUG_ASSERT(
      !containsEntitySparseArray(array, entity),
      "Attempted to add entity to sparse array when entity already existed");

  uint32_t index = entityToPos(entity);

  reserveIndexSparseArray(array, index);

  uint32_t pageIndex = _ECS_GET_PAGE(index);
  uint32_t indexWithinPage = _ECS_INDEX_IN_PAGE(index);

  array->pages[pageIndex][indexWithinPage] = value;
}

void releaseEntitySparseArray(struct SparseArray *array, Entity entity) {
  DEBUG_ASSERT(
      !containsEntitySparseArray(array, entity),
      "Attempted to remove entity from sparse array, but entity never existed");

  uint32_t index = entityToPos(entity);
  uint32_t pageIndex = _ECS_GET_PAGE(index);
  uint32_t indexWithinPage = _ECS_INDEX_IN_PAGE(index);

  // TODO: maybe more correct is just _ECS_NULL
  array->pages[pageIndex][indexWithinPage] = _ECS_NULL | _ECS_DEAD;
}

void freeSparseArray(struct SparseArray *array) {
  for (uint32_t i = 0; i < array->size; i++) {
    free(array->pages[i]);
  }

  free(array->pages);
}

void saveSparseArray(struct SparseArray const *array, FILE *p) {
  // Write page size
  uint32_t pageSize = _ECS_PAGE_SIZE;
  fwrite(&pageSize, sizeof(pageSize), 1, p);
  // Write number of pages
  fwrite(&array->size, sizeof(array->size), 1, p);
  // Iterate pages and write
  for (uint32_t i = 0; i < array->size; i++) {
    Entity *page = array->pages[i];
    DEBUG_ASSERT(page != NULL, "Null page when saving sparse array");

    fwrite(page, sizeof(Entity), _ECS_PAGE_SIZE, p);
  }
}

struct SparseArray loadSparseArray(FILE *p) {
  struct SparseArray array = {0};

  uint32_t pageSize = 0;
  fread(&pageSize, sizeof(pageSize), 1, p);
  DEBUG_ASSERT(pageSize == _ECS_PAGE_SIZE,
               "Loaded page size was different to definition");
  uint32_t arraySize = 0;
  fread(&arraySize, sizeof(arraySize), 1, p);

  reserveIndexSparseArray(&array, (arraySize - 1) * _ECS_PAGE_SIZE);

  for (uint32_t i = 0; i < array.size; i++) {
    Entity *page = array.pages[i];
    DEBUG_ASSERT(page != NULL, "Attempted to write to null page");

    fread(page, sizeof(Entity), _ECS_PAGE_SIZE, p);
  }

  return array;
}
