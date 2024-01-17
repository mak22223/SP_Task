project(sp-task)
cmake_minimum_required(VERSION 3.22)

set(INSTALL_DIRECTORY_PATH install)

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-O2 -Wall -Wpedantic -Wextra)

add_subdirectory("src/master")
add_subdirectory("src/worker")
