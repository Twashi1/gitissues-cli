#ifndef _GITISSUES_ECS_COMPONENT_POOL_H_
#define _GITISSUES_ECS_COMPONENT_POOL_H_

#include <gitissues/ecs/entity.h>
#include <gitissues/ecs/paged_array.h>
#include <gitissues/errs.h>
#include <gitissues/json/json.h>

typedef void (*ComponentMove)(uint8_t *src, uint8_t *dest);
typedef void (*ComponentDelete)(uint8_t *src);
typedef void (*ComponentSwapRemove)(uint8_t *toDelete, uint8_t *toKeep);
typedef void (*ComponentSave)(uint8_t *src, FILE *p);
typedef void (*ComponentLoad)(uint8_t *dest, FILE *p);
typedef enum ErrorCode (*ComponentSaveJson)(FILE *p, uint8_t *src);
typedef enum ErrorCode (*ComponentLoadJson)(struct JsonReader *p,
                                            uint8_t *dest);

struct ComponentManager {
  ComponentMove move;
  ComponentDelete delete;
  ComponentSwapRemove swapRemove;
  ComponentSave save;
  ComponentLoad load;
  ComponentSaveJson saveJson;
  ComponentLoadJson loadJson;
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
  void *userData;
};

struct ComponentPool
createManagedComponentPool(uint32_t sizeOfType,
                           struct ComponentManager manager);
struct ComponentPool createComponentPool(uint32_t sizeOfType);
void *getUserDataComponentPool(struct ComponentPool *pool);
void setUserDataComponentPool(struct ComponentPool *pool, void *userData);
enum ErrorCode reserveComponentPool(struct ComponentPool *pool, uint32_t index);
uint8_t *emplaceEntityToComponentPool(struct ComponentPool *pool,
                                      Entity entity);
enum ErrorCode addEntityToComponentPool(struct ComponentPool *pool,
                                        Entity entity, uint8_t *componentData);
enum ErrorCode freeEntityComponentPool(struct ComponentPool *pool,
                                       Entity entity);
enum ErrorCode popEntityComponentPool(struct ComponentPool *pool);
enum ErrorCode saveEntityJsonComponentPool(struct ComponentPool *pool,
                                           Entity entity, FILE *p);
enum ErrorCode reloadEntityJsonComponentPool(struct ComponentPool *pool,
                                             Entity entity,
                                             struct JsonReader *p);
enum ErrorCode addEntityJsonComponentPool(struct ComponentPool *pool,
                                          Entity entity, struct JsonReader *p);
uint8_t *getEntityComponentPool(struct ComponentPool *pool, Entity entity);
uint8_t *getOrNullEntityComponentPool(struct ComponentPool *pool,
                                      Entity entity);
bool containsEntityComponentPool(struct ComponentPool *pool, Entity entity);
void freeComponentPool(struct ComponentPool *pool);

void saveComponentPool(struct ComponentPool const *pool, FILE *p);
struct ComponentPool loadComponentPool(FILE *p);
struct ComponentPool loadManagedComponentPool(FILE *p,
                                              struct ComponentManager manager);

#endif
