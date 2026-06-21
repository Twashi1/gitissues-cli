cmake_minimum_required(VERSION 3.16)

project(gitissues_cli C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(gitissues
    src/main.c
)

target_compile_options(gitissues PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
