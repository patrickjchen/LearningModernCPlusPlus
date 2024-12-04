cmake_minimum_required(VERSION 3.15)
project(MultiExecutableProject VERSION 1.0 LANGUAGES CXX)

# Set common compiler settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")

# Executable 1
add_executable(single_thread
    src/single_thread.cpp
)
target_compile_options(single_thread PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(single_thread PRIVATE pthread)  # Link pthread library

# Executable 2
add_executable(multi_thread
    src/multi_thread.cpp
)
target_compile_options(multi_thread PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(multi_thread PRIVATE pthread)  # Link pthread library

# Executable 3
add_executable(single_coroutine
    src/single_coroutine.cpp
)
target_compile_options(single_coroutine PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(single_coroutine PRIVATE pthread)  # Link pthread library

# Executable 4
add_executable(multi_coroutine
    src/multi_coroutine.cpp
)
target_compile_options(multi_coroutine PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(multi_coroutine PRIVATE pthread)  # Link pthread library

# Executable 5
add_executable(co
    src/co.cpp
)
target_compile_options(co PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(co PRIVATE pthread)  # Link pthread library
