cmake_minimum_required(VERSION 3.16)

project(gitissues_cli C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(gitissues
    src/main.c
    src/umbra_string.c
)

target_include_directories(gitissues PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_compile_options(gitissues PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
