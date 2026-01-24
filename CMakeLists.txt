cmake_minimum_required(VERSION 3.12)
project(vellum LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_VERSION})")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ compiler flags: ${CMAKE_CXX_FLAGS}")

add_subdirectory(src)
add_subdirectory(test) 

set(LSP_USE_SANITIZERS OFF)
add_subdirectory(external/lsp-framework)
add_subdirectory(lsp-server)