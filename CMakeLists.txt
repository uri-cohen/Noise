cmake_minimum_required(VERSION 4.2.3)

project(noise CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(CTAGS_PATH ctags)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)
