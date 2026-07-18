plugins {
    kotlin("jvm") version "2.2.0"
    kotlin("plugin.serialization") version "2.2.0"
    application
}

repositories {
    mavenCentral()
}

dependencies {
    implementation(project(":bindings"))
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.9.0")
}

application {
    mainClass.set("MainKt")
}

kotlin {
    jvmToolchain(21)
}

tasks.named<JavaExec>("run") {
    // TODO: path sucks
    jvmArgs(
        "-Djava.library.path=${rootProject.projectDir.parentFile}/build/kotlin/kotlin/jni",
    )
}
