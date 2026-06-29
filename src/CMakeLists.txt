# Minimální požadovaná verze CMake
cmake_minimum_required(VERSION 3.10)

# Název projektu a jazyk
project(SDRProgram CXX)

# Vynucení moderního C++ standardu (C++17 je ideální pro std::shared_ptr a vlákna)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Nalezení všech .cpp souborů (ekvivalent k shell find . -name '*.cpp')
file(GLOB_RECURSE SRCS "*.cpp")

# Vytvoření cílového spustitelného souboru
add_executable(program ${SRCS})

# Aplikace vašich flagů a optimalizací specificky pro tento cíl
target_compile_options(program PRIVATE
    -Wall
    -Wextra
    -O3
    -march=native
    -ffast-math
    -fopenmp-simd
)

# Nalezení a připojení podpory pro POSIX vlákna (std::thread)
find_package(Threads REQUIRED)
target_link_libraries(program PRIVATE Threads::Threads)