cmake_minimum_required (VERSION 2.8.12)
project (simple-sqlite C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wno-gnu-pointer-arith -Wall")
endif()

add_executable(simpleSQLite SQLite.c)
