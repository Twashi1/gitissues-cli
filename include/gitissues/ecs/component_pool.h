#ifndef _GITISSUES_ECS_COMPONENT_POOL_H_
#define _GITISSUES_ECS_COMPONENT_POOL_H_

#include <gitissues/ecs/entity.h>
#include <gitissues/ecs/paged_array.h>
#include <gitissues/errs.h>

typedef void (*ComponentMove)(uint8_t *src, uint8_t *dest);
typedef void (*ComponentDelete)(uint8_t *src);
typedef void (*ComponentSwapRemove)(uint8_t *toDelete, uint8_t *toKeep);
typedef void (*ComponentSave)(uint8_t *src, FILE *p);
typedef void (*ComponentLoad)(uint8_t *dest, FILE *p);

struct ComponentManager {
  ComponentMove move;
  ComponentDelete delete;
  ComponentSwapRemove swapRemove;
  ComponentSave save;
  ComponentLoad load;
};

struct ComponentPool {
  struct {
    uint8_t *data;
    Entity *entity;
    uint32_t size;
    uint32_t capacity;
  } dense;

  // Sparse array of entities
  struct SparseArray sparse;
  struct ComponentManager manager;
  uint32_t sizeOfType;
};

struct ComponentPool
createManagedComponentPool(uint32_t sizeOfType,
                           struct ComponentManager manager);
struct ComponentPool createComponentPool(uint32_t sizeOfType);
enum ErrorCode reserveComponentPool(struct ComponentPool *pool, uint32_t index);
// TODO: add into function name "ByMove" or "Move"
// TODO: copy function (uint8_t const*)
enum ErrorCode addEntityToComponentPool(struct ComponentPool *pool,
                                        Entity entity, uint8_t *componentData);
enum ErrorCode freeEntityComponentPool(struct ComponentPool *pool,
                                       Entity entity);
enum ErrorCode popEntityComponentPool(struct ComponentPool *pool);
uint8_t *getEntityComponentPool(struct ComponentPool *pool, Entity entity);
uint8_t *getOrNullEntityComponentPool(struct ComponentPool *pool,
                                      Entity entity);
bool hasEntityComponentPool(struct ComponentPool *pool, Entity entity);
void freeComponentPool(struct ComponentPool *pool);

void saveComponentPool(struct ComponentPool const *pool, FILE *p);
// TODO: need to make a function for loading with manager (abstract)
struct ComponentPool loadComponentPool(FILE *p);

#endif
