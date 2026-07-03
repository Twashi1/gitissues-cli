#include <gitissues/ecs/component_pool.h>
#include <gitissues/ecs/paged_array.h>
#include <gitissues/errs.h>
#include <gitissues/log.h>

struct ComponentPool
createManagedComponentPool(uint32_t sizeOfType,
                           struct ComponentManager manager) {
  struct ComponentPool pool = createComponentPool(sizeOfType);
  pool.manager = manager;

  return pool;
}

struct ComponentPool createComponentPool(uint32_t sizeOfType) {
  struct ComponentPool pool = {0};
  pool.sizeOfType = sizeOfType;
  pool.manager.delete = NULL;
  pool.manager.move = NULL;
  pool.manager.swapRemove = NULL;
  pool.manager.save = NULL;
  pool.manager.load = NULL;
  pool.sparse = createSparseArray();

  return pool;
}

enum ErrorCode reserveComponentPool(struct ComponentPool *pool,
                                    uint32_t index) {
  if (pool->dense.capacity > index)
    return GITISSUES_OK;

  uint32_t growthFactor = pool->dense.capacity + (pool->dense.capacity >> 1);
  uint32_t newCapacity = index >= growthFactor ? index + 1 : growthFactor;

  if (pool->manager.move == NULL) {
    uint8_t *pData = realloc(pool->dense.data, pool->sizeOfType * newCapacity);
    if (pData == NULL) {
      return GITISSUES_OUT_OF_MEMORY;
    }
    pool->dense.data = pData;
  } else {
    uint8_t *pData = malloc(pool->sizeOfType * newCapacity);
    if (pData == NULL) {
      return GITISSUES_OUT_OF_MEMORY;
    }

    // Move each data over
    for (uint32_t i = 0; i < pool->dense.size; i++) {
      pool->manager.move(pool->dense.data + (i * pool->sizeOfType),
                         pData + (i * pool->sizeOfType));
    }
  }

  Entity *pEntity = realloc(pool->dense.entity, sizeof(Entity) * newCapacity);
  if (pEntity == NULL) {
    return GITISSUES_OUT_OF_MEMORY;
  }

  pool->dense.entity = pEntity;
  pool->dense.capacity = newCapacity;

  return GITISSUES_OK;
}

enum ErrorCode addEntityToComponentPool(struct ComponentPool *pool,
                                        Entity entity, uint8_t *componentData) {
  // Add entity to sparse array at position
  uint32_t entityIndex = pool->dense.size; // TODO: associate version?
  enum ErrorCode ec = addEntitySparseArray(&pool->sparse, entity, entityIndex);
  DEBUG_PRINT_ERROR("Adding entity to sparse array %s\n", ec);
  if (ec != GITISSUES_OK)
    return ec;

  GITISSUES_LOG_DEBUG("Added entity, index %d to sparse array", entityIndex);

  ec = reserveComponentPool(pool, entityIndex);
  if (ec != GITISSUES_OK)
    return ec;

  pool->dense.entity[entityIndex] = entity;

  // TODO: can abstract this out better, move to variable
  if (pool->manager.move != NULL) {
    pool->manager.move(componentData,
                       pool->dense.data + (entityIndex * pool->sizeOfType));
  } else {
    memcpy(pool->dense.data + (entityIndex * pool->sizeOfType), componentData,
           pool->sizeOfType);
  }

  pool->dense.size++;

  return GITISSUES_OK;
}

enum ErrorCode freeEntityComponentPool(struct ComponentPool *pool,
                                       Entity entity) {
  uint32_t entityIndex = getElemEntitySparseArray(&pool->sparse, entity);

  // TODO: DEBUG assert entity matches at that position
  uint8_t *component = pool->dense.data + (entityIndex * pool->sizeOfType);
  uint8_t *end = pool->dense.data + ((pool->dense.size - 1) * pool->sizeOfType);

  if (component == end) {
    return popEntityComponentPool(pool);
  }

  // TODO: range-based can be sped up significantly, so many branches
  // Swap remove component with end
  if (pool->manager.swapRemove != NULL) {
    pool->manager.swapRemove(component, end);
  } else {
    // Check: did the user define move, delete?
    if (pool->manager.delete != NULL) {
      pool->manager.delete(component);
    }
    // Allow element to just be overwritten

    // Move with provided move, or just copy bytes
    if (pool->manager.move != NULL) {
      pool->manager.move(end, component);
    } else {
      memcpy(component, end, pool->sizeOfType);
    }
  }

  // Move entity ID over (no swap -- unnecessary)
  pool->dense.entity[entityIndex] = pool->dense.entity[pool->dense.size - 1];
  --(pool->dense.size);

  return GITISSUES_OK;
}

enum ErrorCode popEntityComponentPool(struct ComponentPool *pool) {
  if (pool->manager.delete != NULL) {
    pool->manager.delete(pool->dense.data +
                         (pool->dense.size - 1) * pool->sizeOfType);
  }

  --(pool->dense.size);

  return GITISSUES_OK;
}

void freeComponentPool(struct ComponentPool *pool) {
  if (pool->manager.delete != NULL) {
    for (uint32_t i = 0; i < pool->dense.size; i++) {
      pool->manager.delete(pool->dense.data + (i * pool->sizeOfType));
    }
  }

  free(pool->dense.data);
  free(pool->dense.entity);

  freeSparseArray(&pool->sparse);
}

uint8_t *getEntityComponentPool(struct ComponentPool *pool, Entity entity) {
  // Get index from sparse array
  uint32_t denseIndex = getElemEntitySparseArray(&pool->sparse, entity);
  DEBUG_ASSERT(denseIndex != UINT32_MAX, "Entity not in sparse array");

  return &pool->dense.data[denseIndex * pool->sizeOfType];
}

uint8_t *getOrNullEntityComponentPool(struct ComponentPool *pool,
                                      Entity entity) {
  uint32_t denseIndex = getElemEntitySparseArray(&pool->sparse, entity);
  if (denseIndex == UINT32_MAX)
    return NULL;

  return &pool->dense.data[denseIndex * pool->sizeOfType];
}

bool hasEntityComponentPool(struct ComponentPool *pool, Entity entity) {
  return containsEntitySparseArray(&pool->sparse, entity);
}

void saveComponentPool(struct ComponentPool const *pool, FILE *p) {
  saveSparseArray(&pool->sparse, p);
  fwrite(&pool->sizeOfType, sizeof(pool->sizeOfType), 1, p);
  // TODO: write dense
  fwrite(&pool->dense.size, sizeof(pool->dense.size), 1, p);
  fwrite(pool->dense.entity, sizeof(Entity), pool->dense.size, p);

  if (pool->manager.save != NULL) {
    for (uint32_t i = 0; i < pool->dense.size; i++) {
      pool->manager.save(&pool->dense.data[i * pool->sizeOfType], p);
    }
  } else {
    fwrite(pool->dense.data, sizeof(uint8_t) * pool->sizeOfType,
           pool->dense.size, p);
  }
}
// TODO: need to pass in the component manager
struct ComponentPool loadComponentPool(FILE *p) {
  struct ComponentPool pool = {0};
  pool.manager.save = NULL;
  pool.manager.load = NULL;
  pool.manager.swapRemove = NULL;
  pool.manager.delete = NULL;
  pool.manager.move = NULL;
  pool.sparse = loadSparseArray(p);

  fread(&pool.sizeOfType, sizeof(pool.sizeOfType), 1, p);
  fread(&pool.dense.size, sizeof(pool.dense.size), 1, p);
  pool.dense.capacity = pool.dense.size;

  pool.dense.entity = malloc(sizeof(Entity) * pool.dense.size);
  DEBUG_ASSERT(pool.dense.entity != NULL,
               "Out of memory allocating dense entity");
  pool.dense.data = malloc(sizeof(uint8_t) * pool.sizeOfType * pool.dense.size);
  DEBUG_ASSERT(pool.dense.data != NULL,
               "Out of memory allocating dense components");

  fread(pool.dense.entity, sizeof(Entity), pool.dense.size, p);
  fread(pool.dense.data, sizeof(uint8_t) * pool.sizeOfType, pool.dense.size, p);

  return pool;
}
