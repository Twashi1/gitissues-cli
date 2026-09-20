plugins {
    kotlin("jvm") version "2.2.0"
}

repositories {
    mavenCentral()
}

kotlin {
    jvmToolchain(21)
}

// sourceSets {
//     main {
//         // Including the java code for interfaces/etc.
//         // TODO: might be better to move the files here, or under subdirectory, and generate headers from that
//         java.srcDir("../../jni/java")
//     }
// }

dependencies {
    api(project(":jni"))
}
