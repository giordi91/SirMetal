cmake_minimum_required(VERSION 3.13.0)
project(SirMetal)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# using Clang
set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS}  -Wall -pedantic -Wextra -m64  -mfma -ffast-math")

add_subdirectory(vendors/meshoptimizer)
add_subdirectory(engine)
add_subdirectory(sandbox)
