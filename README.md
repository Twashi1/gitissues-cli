# Gitissues-cli

## Build

### Build C Library (JNI)
Configure and build the C library with JNI bindings.
```sh
cmake --preset jni
cmake --build --preset jni 
```
Alternatively, build the C library as-is.
```sh
cmake --preset default
cmake --build --preset default
```

### Build Java Bindings
```sh
# Build the Java bindings module (produces jar and loads native library)
cd jni 
./gradlew :jni:jar
```

## Description

Will be the C library behind gitissues with Java API.

The library provides:
- High-performance C library (`src/api/`)
- Minimal API Java bindings (in `jni/`)
For high-performance applications, directly interacting with the C ECS system is recommended.
