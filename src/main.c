#include <gitissues/ecs/registry.h>
#include <gitissues/umbra_string.h>
#include <stdio.h>

int main(int argc, char **argv) {
  if (argc > 1) {
    printf("Argument: %s\n", argv[1]);
  }

  printf("Hello, CMake C project\n");

  // testRegistry();

  return 0;
}
