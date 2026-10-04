#include <gitissues/ecs/registry.h>

#include "gitissues/ecs/component_pool.h"
#include "gitissues/log.h"

struct Registry createRegistry(void) {
  struct Registry registry = {0};
  registry.lifetimeAllocations = createBlockAllocator(4096);
  return registry;
}

void freeRegistry(struct Registry *registry) {
  // Free all component pools
  for (uint32_t i = 0; i < registry->pools.size; i++) {
    struct ComponentPool *pool = &registry->pools.data[i];
    freeComponentPool(pool);
  }

  free(registry->pools.data);
  free(registry->createdEntities.data);
  freeBlockAllocator(&registry->lifetimeAllocations);
}

Entity createEntity(struct Registry *registry) {
  // if (registry->availableEntities > 0) {
  //   // Use nextEntity
  //   Entity newEntity = registry->nextEntity;
  //   // Swap nextEntity to update it
  //   Entity nextNextEntity =
  //       registry->createdEntities.data[registry->nextEntity];
  //   registry->createdEntities.data[registry->nextEntity] = newEntity;
  //   registry->nextEntity = nextNextEntity;
  //
  //   registry->availableEntities--;
  //
  //   return newEntity;
  // }

  // TODO: use recycling properly
  Entity e = registry->createdEntities.size;

  if (registry->createdEntities.size + 1 >=
      registry->createdEntities.capacity) {
    uint32_t newCapacity = registry->createdEntities.capacity +
                           (registry->createdEntities.capacity >> 1) + 1;
    void *p =
        realloc(registry->createdEntities.data, newCapacity * sizeof(Entity));
    DEBUG_ASSERT(p != NULL, "Out of memory");
    registry->createdEntities.data = p;
    registry->createdEntities.capacity = newCapacity;
  }

  registry->createdEntities.data[registry->createdEntities.size++] = e;

  return e;
}

bool isEntityNull(struct Registry *registry, Entity entity) {
  if (entityToPos(entity) == _ECS_NULL)
    return true;

  if (entity < registry->createdEntities.size) {
    // Version number match too
    if (registry->createdEntities.data[entityToPos(entity)] == entity) {
      return false;
    }
  }

  return true;
}

void freeEntity(struct Registry *registry, Entity entity) {
  // TODO: highly inefficient
  // Iterate all component pools
  for (uint32_t i = 0; i < registry->componentIDMap.dense.size; i++) {
    struct StringMapNode mapNode = registry->componentIDMap.dense.data[i];
    ComponentID id = mapNode.value;

    struct ComponentPool *pool = &registry->pools.data[id];
    if (containsEntityComponentPool(pool, entity)) {
      freeEntityComponentPool(pool, entity);
    }
  }

  // Increment the implicit list
  // Entity replacement = incrementEntityVersion(registry->nextEntity);
  // TODO: recycling
}

bool isRegistered(struct Registry *registry, struct UmbraString string) {
  ComponentID id = getStringMap(&registry->componentIDMap, string);

  return id != _GITISSUES_COMPONENT_INVALID;
}

ComponentID registerComponentID(struct Registry *registry,
                                struct UmbraString const string,
                                uint32_t sizeOfType) {
  GITISSUES_LOG_DEBUG("Registering component with name %.*s", string.size,
                      getUmbraPtrConst(&string));
  ComponentID id = getStringMap(&registry->componentIDMap, string);

  if (id == _GITISSUES_COMPONENT_INVALID) {
    insertStringMap(&registry->componentIDMap, string, registry->pools.size);
    id = registry->pools.size;

    if (registry->pools.size + 1 >= registry->pools.capacity) {
      uint32_t newCapacity =
          registry->pools.capacity + (registry->pools.capacity >> 1) + 1;
      void *p = realloc(registry->pools.data,
                        newCapacity * sizeof(struct ComponentPool));
      GITISSUES_LOG_DEBUG(
          "Reallocating component pools with capacity: %d, new address %p\n",
          newCapacity, p);
      DEBUG_ASSERT(p != NULL, "Out of memory");
      registry->pools.data = p;
      registry->pools.capacity = newCapacity;
    }

    struct ComponentPool *pool = &registry->pools.data[id];
    *pool = createComponentPool(sizeOfType);
    registry->pools.size++;
  }

  return id;
}

ComponentID getComponentID(struct Registry *registry,
                           struct UmbraString const string) {
  return getStringMap(&registry->componentIDMap, string);
}

void addComponent(struct Registry *registry, Entity entity, ComponentID id,
                  uint8_t *data) {
  struct ComponentPool *pool = &registry->pools.data[id];
  // GITISSUES_LOG_DEBUG("Adding component id %d; ptr: %p to entity %d", id,
  //                     (void *)pool, entity);
  addEntityToComponentPool(pool, entity, data);
}

void removeComponent(struct Registry *registry, Entity entity, ComponentID id) {
  struct ComponentPool *pool = &registry->pools.data[id];
  freeEntityComponentPool(pool, entity);
}

bool hasComponent(struct Registry const *registry, Entity entity,
                  ComponentID id) {
  struct ComponentPool const *pool = &registry->pools.data[id];
  return containsEntityComponentPool(pool, entity);
}

uint8_t *getComponent(struct Registry *registry, Entity entity,
                      ComponentID id) {
  struct ComponentPool *pool = &registry->pools.data[id];
  return getEntityComponentPool(pool, entity);
}

uint8_t const *getComponentConst(struct Registry const *registry, Entity entity,
                                 ComponentID id) {
  struct ComponentPool const *pool = &registry->pools.data[id];
  return getEntityComponentPoolConst(pool, entity);
}

uint8_t *getOrNullComponent(struct Registry *registry, Entity entity,
                            ComponentID id) {
  struct ComponentPool *pool = &registry->pools.data[id];
  return getOrNullEntityComponentPool(pool, entity);
}

struct ComponentPool *getPool(struct Registry *registry, ComponentID id) {
  return &registry->pools.data[id];
}

void setUserData(struct Registry *registry, ComponentID id, void *userData) {
  getPool(registry, id)->userData = userData;
}

void *getUserData(struct Registry *registry, ComponentID id) {
  return getPool(registry, id)->userData;
}

void saveRegistry(struct Registry const *registry, FILE *p) {
  fwrite(&registry->createdEntities.size,
         sizeof(registry->createdEntities.size), 1, p);
  fwrite(registry->createdEntities.data, sizeof(Entity),
         registry->createdEntities.size, p);

  fwrite(&registry->nextEntity, sizeof(registry->nextEntity), 1, p);
  fwrite(&registry->availableEntities, sizeof(registry->availableEntities), 1,
         p);

  saveStringMap(&registry->componentIDMap, p);

  fwrite(&registry->pools.size, sizeof(registry->pools.size), 1, p);
  DEBUG_ASSERT(registry->pools.size == 0 || registry->pools.data != NULL,
               "Registry pools was NULL when size was non-zero");
  for (uint32_t i = 0; i < registry->pools.size; i++) {
    saveComponentPool(&registry->pools.data[i], p);
  }
}

struct Registry loadRegistry(FILE *p) {
  struct Registry registry = {0};

  fread(&registry.createdEntities.size, sizeof(registry.createdEntities.size),
        1, p);
  registry.createdEntities.data =
      malloc(sizeof(Entity) * registry.createdEntities.size);
  DEBUG_ASSERT(registry.createdEntities.data != NULL,
               "Out of memory allocating registry created entities");
  fread(registry.createdEntities.data, sizeof(Entity),
        registry.createdEntities.size, p);

  fread(&registry.nextEntity, sizeof(registry.nextEntity), 1, p);
  fread(&registry.availableEntities, sizeof(registry.availableEntities), 1, p);

  registry.componentIDMap = loadStringMap(p);

  fread(&registry.pools.size, sizeof(registry.pools.size), 1, p);
  registry.pools.data =
      malloc(sizeof(struct ComponentPool) * registry.pools.size);
  DEBUG_ASSERT(registry.pools.data != NULL,
               "Out of memory allocating registry pools");
  for (uint32_t i = 0; i < registry.pools.size; i++) {
    registry.pools.data[i] = loadComponentPool(p);
  }

  registry.createdEntities.capacity = registry.createdEntities.size;
  registry.pools.capacity = registry.pools.size;

  return registry;
}

void saveEntityJson(struct Registry *registry, Entity entity, FILE *p) {
  // We have no guarantee on ordering of component pools, so we have to store
  // the tag names
  jsonWriteObjectBegin(p);
  bool writtenOne = false;

  for (uint32_t i = 0; i < registry->pools.size; i++) {
    struct ComponentPool *pool = &registry->pools.data[i];

    if (!containsEntityComponentPool(pool, entity))
      continue;

    if (writtenOne) {
      jsonWriteNext(p);
    }

    writtenOne = true;

    struct UmbraString componentName =
        registry->componentIDMap.dense.data[i].string;
    jsonWriteKeyUmbra(componentName, p);

    saveEntityJsonComponentPool(pool, entity, p);
  }

  jsonWriteObjectEnd(p);
}

// Load new entity
Entity loadEntityJson(struct Registry *registry, struct JsonReader *p) {
  jsonReadObjectBegin(p);

  // assume user saves an entity;
  // - we need to know which components the entity has; thus we do need the
  // component strings
  // - but what about creating the component pools; assuming they're not already
  // created how do managers work?
  // - user creates an association between components and custom management
  // methods in their code?
  // - somehow we assume user provides these callbacks

  // -> should user only be able to save/load whole registry?
  // -> we also want user to be able to translate issues into arbitrary JSON
  // (currently saved to file, but ideally also a string stream)
  // -> user must also be able to create new issues from scratch by loading
  // arbitrary JSON
  // -> but then user must provide the manager for each component (let's assume
  // they've registers the relevant component pools)
  Entity entity = createEntity(registry);

  char next = jsonPeekNext(p);
  for (; next != '\0' && next != '}'; next = jsonPeekNext(p)) {
    char *key;

    jsonReadKeyLifetime(p, &registry->lifetimeAllocations, &key);

    struct UmbraString keyString = {0};
    createUmbraStringParasitic(&keyString, key);

    ComponentID id = getComponentID(registry, keyString);
    DEBUG_ASSERT(isRegistered(registry, keyString),
                 "Expected component to already be registered before loading "
                 "entity with that component");
    struct ComponentPool *pool = &registry->pools.data[id];

    // We assume user already registers the component pool somehow
    addEntityJsonComponentPool(pool, entity, p);
  }

  jsonReadObjectEnd(p);

  return entity;
}

void reloadEntityJson(struct Registry *registry, Entity entity,
                      struct JsonReader *p) {
  jsonReadObjectBegin(p);

  char next = jsonPeekNext(p);
  for (; next != '\0' && next != '}'; next = jsonPeekNext(p)) {
    char *key;

    jsonReadKeyLifetime(p, &registry->lifetimeAllocations, &key);

    struct UmbraString keyString = {0};
    createUmbraStringParasitic(&keyString, key);

    ComponentID id = getComponentID(registry, keyString);
    DEBUG_ASSERT(isRegistered(registry, keyString),
                 "Expected component to already be registered before loading "
                 "entity with that component");
    struct ComponentPool *pool = &registry->pools.data[id];

    // We assume user already registers the component pool somehow
    reloadEntityJsonComponentPool(pool, entity, p);
  }

  jsonReadObjectEnd(p);
}

struct PoolIterator iterateComponentPool(struct Registry *registry,
                                         ComponentID id) {
  struct PoolIterator iterator;
  struct ComponentPool *pool = getPool(registry, id);
  iterator.start = pool->dense.data;
  iterator.count = pool->dense.size;
  iterator.sizeOfType = pool->sizeOfType;
  iterator.registry = registry;

  return iterator;
}

struct EntityGroupIterator iterateComponentGroup(struct Registry *registry,
                                                 ComponentID *ids,
                                                 uint32_t numIds) {
  struct EntityGroupIterator iterator;
  iterator.registry = registry;

  if (numIds == 0 || ids == NULL) {
    iterator.entities = NULL;
    iterator.entityCount = 0;
    iterator.ids = NULL;
    iterator.idCount = 0;

    return iterator;
  }

  iterator.idCount = numIds;
  iterator.ids = malloc(sizeof(ComponentID) * numIds);
  DEBUG_ASSERT(iterator.ids != NULL,
               "Failed to allocate for iterator component ids");
  memcpy(iterator.ids, ids, numIds * sizeof(ComponentID));

  // Find all pools, select the smallest one, and iterate those entities
  struct ComponentPool *pool = NULL;
  uint32_t poolMinimumEntities = UINT32_MAX;

  for (uint32_t i = 0; i < numIds; i++) {
    ComponentID id = ids[i];

    struct ComponentPool *candidate = getPool(registry, id);
    DEBUG_ASSERT(candidate != NULL, "Candidate pool was null");

    if (candidate->dense.size < poolMinimumEntities) {
      pool = candidate;
    }
  }

  // Iterate entities of smallest pool
  // We'll be lazy with allocation here and allocate more than we need
  iterator.entities = malloc(pool->dense.size * sizeof(Entity));
  iterator.entityCount = 0;
  DEBUG_ASSERT(iterator.entities != NULL,
               "Failed to allocate memory for iterator entity storage");

  for (uint32_t i = 0; i < pool->dense.size; i++) {
    Entity entity = pool->dense.entity[i];

    bool signatureIsSubset = true;

    for (uint32_t j = 0; j < numIds; j++) {
      ComponentID id = ids[j];

      if (!hasComponent(registry, entity, id)) {
        signatureIsSubset = false;
        break;
      }
    }

    if (signatureIsSubset) {
      iterator.entities[iterator.entityCount++] = entity;
    }
  }

  return iterator;
}

void freeComponentGroupIterator(struct EntityGroupIterator *iterator) {
  free(iterator->entities);
  free(iterator->ids);
}
