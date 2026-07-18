plugins {
    `java-library`
}

group = "gitissues"
version = "1.0"

java {
    toolchain {
        languageVersion.set(JavaLanguageVersion.of(21))
    }
}

sourceSets {
    main {
        java {
            srcDir("java/src/main/java")
        }
    }
}

tasks.compileJava {
    options.compilerArgs.addAll(
        listOf(
            "-h",
            "$projectDir/build/generated",
        ),
    )
}
