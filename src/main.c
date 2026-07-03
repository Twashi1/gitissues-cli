#include <gitissues/ecs/registry.h>
#include <gitissues/umbra_string.h>
#include <stdio.h>

void testRegistry(void) {
  struct Registry reg = createRegistry();

  struct BlockAllocator allocator = createBlockAllocator(4096);

  // Track entities
  Entity *e = malloc(sizeof(Entity) * 1000);

  struct UmbraString fizz;
  setImmutableString(&fizz, "Fizz");
  struct UmbraString buzz;
  setModifiableString(&buzz, "Buzz", &allocator);

  printf("Setup umbra strings\n");

  ComponentID id = registerComponentID(&reg, fizz, sizeof(int));

  printf("Registered first componnet\n");
  ComponentID id2 = registerComponentID(&reg, buzz, sizeof(float));

  printf("Registered components: %d %d\n", id, id2);

  for (uint32_t i = 0; i < 1000; i++) {
    e[i] = createEntity(&reg);

    printf("Created entity: %d, value: %d\n", i, e[i]);

    if (i % 3 == 0) {
      ComponentID fizzID = getComponentID(&reg, fizz);
      printf("Component ID Fizz: %d\n", fizzID);
      int fizzValue = i;
      addComponent(&reg, e[i], fizzID, (uint8_t *)&fizzValue);
    }

    if (i % 5 == 0) {
      ComponentID buzzID = getComponentID(&reg, buzz);
      printf("Component ID Buzz: %d\n", buzzID);
      float buzzValue = i;
      addComponent(&reg, e[i], buzzID, (uint8_t *)&buzzValue);
    }
  }

  printf("Added all components\n");

  // Print FizzBuzz
  for (uint32_t i = 0; i < 1000; i++) {
    char buf[128] = {0};

    int *fizzPtr =
        (int *)getOrNullComponent(&reg, e[i], getComponentID(&reg, fizz));
    float *buzzPtr =
        (float *)getOrNullComponent(&reg, e[i], getComponentID(&reg, buzz));

    int fizzValue = -1;
    float buzzValue = -1.0f;

    if (fizzPtr != NULL) {
      strlcat(buf, "Fizz", sizeof(buf));
      fizzValue = *fizzPtr;
    }

    if (buzzPtr != NULL) {
      strlcat(buf, "Buzz", sizeof(buf));
      buzzValue = *buzzPtr;
    }

    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " [%d] [%f]",
             fizzValue, buzzValue);
    printf("%d: %s\n", i, buf);
  }

  free(e);
  freeRegistry(&reg);
}

int main(int argc, char **argv) {
  if (argc > 1) {
    printf("Argument: %s\n", argv[1]);
  }

  printf("Hello, CMake C project\n");

  // testRegistry();

  return 0;
}
