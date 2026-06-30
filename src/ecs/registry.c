#include <gitissues/ecs/registry.h>

#include "gitissues/errs.h"

struct Registry createRegistry(void) {
    struct Registry registry = {0};
    return registry;
}

void freeRegistry(struct Registry* registry) {
    // Free all component pools
    for (uint32_t i = 0; i < registry->pools.size; i++) {
        struct ComponentPool* pool = &registry->pools.data[i];
        freeComponentPool(pool);
    }

    free(registry->pools.data);
    free(registry->createdEntities.data);
}

Entity createEntity(struct Registry* registry) {
    // TODO: use recycling properly
    Entity e = registry->createdEntities.size;

    if (registry->createdEntities.size + 1 >= registry->createdEntities.capacity) {
        uint32_t newCapacity =
            registry->createdEntities.capacity + (registry->createdEntities.capacity >> 1) + 1;
        void* p = realloc(registry->createdEntities.data, newCapacity * sizeof(Entity));
        DEBUG_ASSERT(p != NULL, "Out of memory");
        registry->createdEntities.data = p;
        registry->createdEntities.capacity = newCapacity;
    }

    registry->createdEntities.data[registry->createdEntities.size++] = e;

    return e;
}

// TODO: how do we know what components to free for this entity? just checking every component
// pool?
// TODO: make this do something
void freeEntity(struct Registry* registry, Entity entity) {
}

ComponentID registerComponentID(struct Registry* registry, struct UmbraString const string,
                                uint32_t sizeOfType) {
    printf("Registering component\n ");
    ComponentID id = getStringMap(&registry->componentIDMap, string);

    printf("Grabbed from string map\n");

    if (id == _GITISSUES_COMPONENT_INVALID) {
        enum ErrorCode ec =
            insertStringMap(&registry->componentIDMap, string, registry->pools.size);
        DEBUG_ASSERT(ec == GITISSUES_OK, "Failed to insert string into map");
        id = registry->pools.size;

        if (registry->pools.size + 1 >= registry->pools.capacity) {
            uint32_t newCapacity = registry->pools.capacity + (registry->pools.capacity >> 1) + 1;
            void* p = realloc(registry->pools.data, newCapacity * sizeof(struct ComponentPool));
            printf("Reallocating component pools with capacity: %d, new address %p\n", newCapacity,
                   p);
            DEBUG_ASSERT(p != NULL, "Out of memory");
            registry->pools.data = p;
            registry->pools.capacity = newCapacity;
        }

        struct ComponentPool* pool = &registry->pools.data[id];
        *pool = createComponentPool(sizeOfType);
        registry->pools.size++;
    }

    return id;
}

ComponentID getComponentID(struct Registry* registry, struct UmbraString const string) {
    return getStringMap(&registry->componentIDMap, string);
}

void addComponent(struct Registry* registry, Entity entity, ComponentID id, uint8_t* data) {
    printf("Registry pools size %d, ptr %p, accessing id %d\n", registry->pools.size,
           (void*)registry->pools.data, id);
    struct ComponentPool* pool = &registry->pools.data[id];
    printf("Adding component id %d; ptr: %p to entity %d\n", id, (void*)pool, entity);
    enum ErrorCode ec = addEntityToComponentPool(pool, entity, data);
    DEBUG_PRINT_ERROR("Add entity to component pool error: %s\n", ec);
    DEBUG_ASSERT(ec == GITISSUES_OK, "Adding entity to component pool failed");
    printf("Success\n");
}

void removeComponent(struct Registry* registry, Entity entity, ComponentID id) {
    struct ComponentPool* pool = &registry->pools.data[id];
    enum ErrorCode ec = freeEntityComponentPool(pool, entity);
    DEBUG_ASSERT(ec == GITISSUES_OK, "Removing entity from component pool failed");
}

bool hasComponent(struct Registry* registry, Entity entity, ComponentID id) {
    struct ComponentPool* pool = &registry->pools.data[id];
    return hasEntityComponentPool(pool, entity);
}

uint8_t* getComponent(struct Registry* registry, Entity entity, ComponentID id) {
    struct ComponentPool* pool = &registry->pools.data[id];
    return getEntityComponentPool(pool, entity);
}

uint8_t* getOrNullComponent(struct Registry* registry, Entity entity, ComponentID id) {
    struct ComponentPool* pool = &registry->pools.data[id];
    return getOrNullEntityComponentPool(pool, entity);
}

struct ComponentPool const* getPool(struct Registry* registry, ComponentID id) {
    return &registry->pools.data[id];
}
