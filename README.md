# Gitissues-cli

## Build

### Build C Library (JNI)
```sh
# Configure and build the C library with JNI bindings
cmake --preset kotlin
cmake --build --preset kotlin
```

### Build Java Bindings
```sh
# Build the Java bindings module (produces jar and loads native library)
cd kotlin
./gradlew :jni:jar
```

### Build Kotlin API
```sh
# Build the Kotlin bindings module (depends on Java bindings)
./gradlew :bindings:jar
```

## Description

Will be the C library behind gitissues with Java and Kotlin bindings. The CLI has been removed; this repository now focuses on the core library and language bindings.

The library provides:
- C core functionality (`gitissues_jni`)
- Java bindings (in `kotlin/jni`)
- Kotlin API wrapper (in `kotlin/bindings`)