cmake_minimum_required(VERSION 3.10)
project(FMCenterDemodulator)

# Require C++17 (or change to C++11 if you prefer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/**/*.cpp")

# Add the executable
add_executable(program ${SOURCES})