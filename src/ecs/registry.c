#include <gitissues/ecs/registry.h>

#include "gitissues/errs.h"
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
  GITISSUES_LOG_DEBUG("Registering component with prefix %.4s", &string.prefix);
  ComponentID id = getStringMap(&registry->componentIDMap, string);

  if (id == _GITISSUES_COMPONENT_INVALID) {
    enum ErrorCode ec = insertStringMap(&registry->componentIDMap, string,
                                        registry->pools.size);
    DEBUG_ASSERT(ec == GITISSUES_OK, "Failed to insert string into map");
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
  GITISSUES_LOG_DEBUG("Adding component id %d; ptr: %p to entity %d", id,
                      (void *)pool, entity);
  enum ErrorCode ec = addEntityToComponentPool(pool, entity, data);
  DEBUG_PRINT_ERROR("Add entity to component pool error: %s", ec);
  DEBUG_ASSERT(ec == GITISSUES_OK, "Adding entity to component pool failed");
}

void removeComponent(struct Registry *registry, Entity entity, ComponentID id) {
  struct ComponentPool *pool = &registry->pools.data[id];
  enum ErrorCode ec = freeEntityComponentPool(pool, entity);
  DEBUG_ASSERT(ec == GITISSUES_OK,
               "Removing entity from component pool failed");
}

bool hasComponent(struct Registry *registry, Entity entity, ComponentID id) {
  struct ComponentPool *pool = &registry->pools.data[id];
  return containsEntityComponentPool(pool, entity);
}

uint8_t *getComponent(struct Registry *registry, Entity entity,
                      ComponentID id) {
  struct ComponentPool *pool = &registry->pools.data[id];
  return getEntityComponentPool(pool, entity);
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
    createUmbraStringLifetime(&keyString, key);

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
    createUmbraStringLifetime(&keyString, key);

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
