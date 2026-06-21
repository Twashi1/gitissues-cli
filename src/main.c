#include <stdio.h>

int main(int argc, char **argv) {
  if (argc > 1) {
    printf("Argument: %s\n", argv[1]);
  }

  printf("Hello, CMake C project\n");
  return 0;
}
