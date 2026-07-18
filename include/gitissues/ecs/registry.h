#ifndef _GITISSUES_ECS_REGISTRY_H_
#define _GITISSUES_ECS_REGISTRY_H_

#include <gitissues/ecs/component_pool.h>
#include <gitissues/ecs/entity.h>
#include <gitissues/ecs/string_map.h>
#include <stdio.h>

// TODO: implement recycling
struct Registry {
  struct {
    Entity *data;
    uint32_t size;
    uint32_t capacity;
  } createdEntities;

  Entity nextEntity;
  uint32_t availableEntities;
  struct BlockAllocator lifetimeAllocations;
  struct ImplicitAllocator transientAllocations;

  struct StringMap componentIDMap;
  struct {
    struct ComponentPool *data;
    uint32_t size;
    uint32_t capacity;
  } pools;
};

// TODO: want paged array that maps from uint32_t (tag name hash) -> uint16_t
// (component pool index) uint32_t -> 4 billion elements, well-distributed thus
// not apt for a sparse array so we just use a regular map, but handmade
//  - ideally; want to store this as some type identity
struct Registry createRegistry(void);
void freeRegistry(struct Registry *registry);

Entity createEntity(struct Registry *registry);
void freeEntity(struct Registry *registry, Entity entity);

bool isRegistered(struct Registry *registry, struct UmbraString string);
ComponentID registerComponentID(struct Registry *registry,
                                struct UmbraString const string,
                                uint32_t sizeOfType);
ComponentID getComponentID(struct Registry *registry,
                           struct UmbraString const string);
void addComponent(struct Registry *registry, Entity entity, ComponentID id,
                  uint8_t *data);
void removeComponent(struct Registry *registry, Entity entity, ComponentID id);
uint8_t *getComponent(struct Registry *registry, Entity entity, ComponentID id);
uint8_t *getOrNullComponent(struct Registry *registry, Entity entity,
                            ComponentID id);
bool hasComponent(struct Registry *registry, Entity entity, ComponentID id);
struct ComponentPool *getPool(struct Registry *registry, ComponentID id);

void setUserData(struct Registry *registry, ComponentID id, void *userData);
void *getUserData(struct Registry *registry, ComponentID id);

void saveRegistry(struct Registry const *registry, FILE *p);
struct Registry loadRegistry(FILE *p);

void saveEntityJson(struct Registry *registry, Entity entity, FILE *p);
// Load new entity
Entity loadEntityJson(struct Registry *registry, struct JsonReader *p);
void reloadEntityJson(struct Registry *registry, Entity entity,
                      struct JsonReader *p);

#endif
