cmake_minimum_required(VERSION 3.10)

# Project Name
project(main_benchmark)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include FetchContent module
include(FetchContent)

# Download Google Benchmark
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.7.1  # Use the latest stable release or a specific tag
)
set(BENCHMARK_ENABLE_TESTING OFF)          # Disable Google Benchmark's internal tests
FetchContent_MakeAvailable(benchmark)

# Add your benchmark source file
add_executable(main_benchmark main_benchmark.cpp)

# Link Google Benchmark with your target
target_link_libraries(main_benchmark PRIVATE benchmark::benchmark)

# Optional: Add optimization flags for better performance in benchmarks
target_compile_options(main_benchmark PRIVATE -O3)

