cmake_minimum_required(VERSION 3.16)

project(gitissues_cli C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(GITISSUES_SOURCES
    src/umbra_string.c
    src/tag.c
    src/issue.c
    src/allocator.c
    src/ecs/component_pool.c
    src/ecs/entity.c
    src/ecs/paged_array.c
    src/ecs/registry.c
    src/ecs/string_map.c
    src/tests/ecs.c
    src/tests/json.c
    src/json/json.c
)

add_library(gitissues_lib ${GITISSUES_SOURCES})
string(LENGTH "${CMAKE_SOURCE_DIR}" SOURCE_DIR_LENGTH)
target_compile_definitions(gitissues_lib PUBLIC 
  GITISSUES_SOURCE_DIR_LENGTH=${SOURCE_DIR_LENGTH}
)
target_include_directories(gitissues_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(gitissues_lib PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
target_link_libraries(gitissues_lib PRIVATE m)

add_executable(gitissues src/main.c )
target_link_libraries(gitissues PRIVATE gitissues_lib)

add_executable(gitissues_tests src/tests/test.c)
target_link_libraries(gitissues_tests PRIVATE gitissues_lib)



