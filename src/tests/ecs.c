#include "gitissues/ecs/string_map.h"
#include "gitissues/tests/test.h"
#include <gitissues/ecs/registry.h>
#include <gitissues/tests/ecs.h>

static inline int cmp_int(void const *a, void const *b) {
  int x = *(int const *)a;
  int y = *(int const *)b;

  return (x > y) - (x < y);
}

static void registerComponents(struct ECSContext *ctx) {
  pushHeader(&ctx->suite, "Register components");

  {
    pushTest(&ctx->suite, "Register integer component");
    ComponentID id =
        registerComponentID(&ctx->registry, ctx->fizz, sizeof(int));
    TEST_PASS_CONDITION(&ctx->suite, id != _GITISSUES_COMPONENT_INVALID);

    pushTest(&ctx->suite, "Register float component");
    ComponentID id2 =
        registerComponentID(&ctx->registry, ctx->buzz, sizeof(float));
    TEST_PASS_CONDITION(&ctx->suite, id2 != _GITISSUES_COMPONENT_INVALID);
  }

  popHeader(&ctx->suite);
}

static void checkEntityUniqueness(struct ECSContext *ctx) {
  pushTest(&ctx->suite, "All created entities are unique");
  Entity *entityCopy = malloc(sizeof(Entity) * ctx->entityArraySize);
  memcpy(entityCopy, ctx->entities, sizeof(Entity) * ctx->entityArraySize);

  qsort(entityCopy, ctx->entityArraySize, sizeof(Entity), cmp_int);
  // count unique
  uint32_t unique = 0;
  uint32_t last = _ECS_NULL;
  for (uint32_t i = 0; i < ctx->entityArraySize; i++) {
    if (entityCopy[i] == last)
      continue;

    last = entityCopy[i];
    unique++;
  }

  TEST_PASS_CONDITION(&ctx->suite, unique == ctx->entityArraySize);

  free(entityCopy);
}

static void addEntityComponents(struct ECSContext *ctx) {
  pushHeader(&ctx->suite, "Creating and populating entities");
  // Track entities
  ctx->entities = malloc(sizeof(Entity) * ctx->entityArraySize);

  bool allEntitiesValid = true;

  for (uint32_t i = 0; i < ctx->entityArraySize; i++) {
    ctx->entities[i] = createEntity(&ctx->registry);
    allEntitiesValid = allEntitiesValid && ctx->entities[i] != _ECS_NULL;

    if (i % 3 == 0) {
      ComponentID fizzID = getComponentID(&ctx->registry, ctx->fizz);
      int fizzValue = i;
      addComponent(&ctx->registry, ctx->entities[i], fizzID,
                   (uint8_t *)&fizzValue);
    }

    if (i % 5 == 0) {
      ComponentID buzzID = getComponentID(&ctx->registry, ctx->buzz);
      float buzzValue = i;
      addComponent(&ctx->registry, ctx->entities[i], buzzID,
                   (uint8_t *)&buzzValue);
    }
  }

  pushTest(&ctx->suite, "All created entities are non-null");
  TEST_PASS_CONDITION(&ctx->suite, allEntitiesValid);

  checkEntityUniqueness(ctx);

  popHeader(&ctx->suite);
}

static void componentTests(struct ECSContext *ctx) {
  pushHeader(&ctx->suite, "Basic component checks");

  pushTest(&ctx->suite,
           "Components belong to exactly entities they were added to");

  bool firstComponentExact = true;
  bool secondComponentExact = true;
  bool firstComponentData = true;
  bool secondComponentData = true;

  for (uint32_t i = 0; i < ctx->entityArraySize; i++) {
    int *fizzPtr =
        (int *)getOrNullComponent(&ctx->registry, ctx->entities[i],
                                  getComponentID(&ctx->registry, ctx->fizz));
    float *buzzPtr =
        (float *)getOrNullComponent(&ctx->registry, ctx->entities[i],
                                    getComponentID(&ctx->registry, ctx->buzz));

    if ((i % 3 == 0) != (fizzPtr != NULL)) {
      firstComponentExact = false;
    }

    if ((i % 5 == 0) != (buzzPtr != NULL)) {
      secondComponentExact = false;
    }

    if (fizzPtr && (*fizzPtr != (int)i)) {
      firstComponentData = false;
    }

    if (buzzPtr && (*buzzPtr != (float)i)) {
      secondComponentData = false;
    }
  }

  TEST_PASS_CONDITION(&ctx->suite, firstComponentExact && secondComponentExact);
  pushTest(&ctx->suite, "Components have correct data");
  TEST_PASS_CONDITION(&ctx->suite, firstComponentData && secondComponentData);

  popHeader(&ctx->suite);
}

static void serialiseRegistry(struct ECSContext *ctx) {
  pushHeader(&ctx->suite, "Saving and loading registry");

  pushTest(&ctx->suite, "Saving registry");
  FILE *p = fopen("registry.txt", "wb");
  DEBUG_ASSERT(p != NULL, "Failed to open file for saving registry");
  saveRegistry(&ctx->registry, p);
  fclose(p);
  testPassed(&ctx->suite, "Saved registry to registry.txt");

  pushTest(&ctx->suite, "Loading registry");
  FILE *q = fopen("registry.txt", "rb");
  struct Registry reg2 = loadRegistry(q);
  fclose(q);
  testPassed(&ctx->suite, "Loaded registry from registry.txt");

  // Re-run component tests, but with this registry instead
  struct Registry oldReg = ctx->registry;
  ctx->registry = reg2;

  componentTests(ctx);

  ctx->registry = oldReg;

  freeRegistry(&reg2);

  popHeader(&ctx->suite);
}

void testECS(void) {
  struct ECSContext ctx;

  ctx.suite = createSuite("ECS suite");
  pushHeader(&ctx.suite, "Basic operations");

  ctx.registry = createRegistry();
  ctx.allocator = createBlockAllocator(4096);

  setImmutableString(&ctx.fizz, "Fizz");
  setModifiableString(&ctx.buzz, "Buzz", &ctx.allocator);

  registerComponents(&ctx);

  ctx.entityArraySize = 1000;

  addEntityComponents(&ctx);

  componentTests(&ctx);

  popHeader(&ctx.suite);

  serialiseRegistry(&ctx);

  free(ctx.entities);
  freeRegistry(&ctx.registry);
  freeSuite(&ctx.suite);
}
