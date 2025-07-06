cmake_minimum_required(VERSION 3.10)
project(vellum-lsp)

set(CMAKE_CXX_STANDARD 20)

add_executable(vellum-lsp main.cpp)
target_link_libraries(vellum-lsp PUBLIC lsp)

