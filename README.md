# Gitissues-cli

## Build

```sh
# Build library/JNI bindings
cmake --preset kotlin
cmake --build --preset kotlin

# Run Kotlin CLI
cd kotlin
./gradlew :cli:run
```

## Description

Will be the CLI/C library behind gitissues. Planning to have both Kotlin and Python bindings.
