find_package(JNI REQUIRED)
find_package(Java REQUIRED COMPONENTS Development)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(JNI_HEADER_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/generated)
set(JNI_STUB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/java/src/main/java/gitissues/jni/GitIssues.java ${CMAKE_CURRENT_SOURCE_DIR}/java/src/main/java/gitissues/jni/GitIssuesCodec.java)

add_custom_command(
  OUTPUT
    ${JNI_HEADER_OUTPUT_DIR}/gitissues_GitIssues.h

  COMMAND
    ${CMAKE_COMMAND} -E make_directory
    ${JNI_HEADER_OUTPUT_DIR}

  COMMAND
    ${Java_JAVAC_EXECUTABLE}
    -h
    ${JNI_HEADER_OUTPUT_DIR}
    ${JNI_STUB_SOURCES}

  DEPENDS
    ${JNI_STUB_SOURCES}

  COMMENT
    "Generating JNI headers from Java stubs"
)

add_custom_target(generate_jni_headers DEPENDS ${JNI_HEADER_OUTPUT_DIR}/gitissues_GitIssues.h)

add_library(gitissues_jni SHARED src/gitissues_jni.c ${JNI_HEADER_OUTPUT_DIR}/gitissues_GitIssues.h)
add_dependencies(gitissues_jni generate_jni_headers)

target_include_directories(gitissues_jni
    PRIVATE
        ${JNI_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include
        ${JNI_HEADER_OUTPUT_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gitissues_jni PRIVATE gitissues_lib ${JNI_LIBRARIES})
