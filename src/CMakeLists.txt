cmake_minimum_required(VERSION 3.10)

project(SDRProgram CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SRCS "*.cpp")

add_executable(program ${SRCS})

target_compile_options(program PRIVATE
    -Wall
    -Wextra
    -O3
    -march=native
    -ffast-math
    -fopenmp-simd
)

find_package(Threads REQUIRED)
target_link_libraries(program PRIVATE Threads::Threads)