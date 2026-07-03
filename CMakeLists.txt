cmake_minimum_required(VERSION 3.16)

project(gitissues_cli C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_executable(gitissues
    src/main.c
    src/umbra_string.c
    src/tag.c
    src/issue.c
    src/allocator.c
    src/ecs/component_pool.c
    src/ecs/entity.c
    src/ecs/paged_array.c
    src/ecs/registry.c
    src/ecs/string_map.c
    src/tests/test.c
)

string(LENGTH "${CMAKE_SOURCE_DIR}" SOURCE_DIR_LENGTH)

target_compile_definitions(gitissues PRIVATE
  GITISSUES_SOURCE_DIR_LENGTH=${SOURCE_DIR_LENGTH}
)

target_include_directories(gitissues PUBLIC ${CMAKE_SOURCE_DIR}/include)
# Link maths
target_link_libraries(gitissues PRIVATE m)

target_compile_options(gitissues PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
