plugins {
    `java-library`
}

group = "gitissues"
version = "1.0"

java {
    toolchain {
        languageVersion.set(JavaLanguageVersion.of(21))
    }

    // Add test source sets
    sourceSets {
        test {
            java {
                srcDirs("src/test/java")
            }
            resources {
                srcDirs("src/test/resources")
            }
        }
    }
}

// Configure Java compilation to generate JNI headers
tasks.compileJava {
    options.compilerArgs.addAll(listOf("-h", "$projectDir/build/generated"))
}

// Make sure the compileJava task waits for the native library to be built
// This will be handled by the server project depending on buildNativeLibrary

// Configure test task to use built native library
tasks.test {
    val buildDir =
        projectDir
            .parentFile
            .parentFile
            .resolve("gitissues-cli/build")
            .absolutePath
    val jniDir =
        projectDir
            .parentFile
            .parentFile
            .resolve("gitissues-cli/build/jni/jni")
            .absolutePath

    systemProperty("java.library.path", buildDir + ":" + jniDir)
    useJUnitPlatform()
}

// Add JUnit 5 as a test dependency
dependencies {
    testImplementation("org.junit.jupiter:junit-jupiter:5.10.0")
    testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}
