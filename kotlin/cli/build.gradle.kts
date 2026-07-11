plugins {
    kotlin("jvm")
    application
}

repositories {
    mavenCentral()
}

dependencies {
    implementation(project(":bindings"))
}

application {
    mainClass.set("MainKt")
}

kotlin {
    jvmToolchain(21)
}

tasks.named<JavaExec>("run") {
    jvmArgs(
        "-Djava.library.path=${rootProject.projectDir.parentFile}/build/kotlin/jni",
    )
}
