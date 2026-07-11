find_package(JNI REQUIRED)

add_library(gitissues_jni SHARED src/gitissues_jni.c)

target_include_directories(gitissues_jni
    PRIVATE
        ${JNI_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gitissues_jni PRIVATE gitissues_lib ${JNI_LIBRARIES})

