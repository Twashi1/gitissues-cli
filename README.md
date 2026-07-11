# Gitissues-cli

## Build

```sh
mkdir build
cd build
cmake --preset kotlin .. # Build with JNI bindings (for kotlin/java support), or use --preset default
cmake --build .
cd kotlin
cmake --build .
```

## Description

Will be the CLI/C library behind gitissues. Planning to have both Kotlin and Python bindings.
