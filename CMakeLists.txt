cmake_minimum_required (VERSION 3.12)
project ("vellum")
add_subdirectory(src)

set(CMAKE_CXX_STANDARD 23)

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_VERSION})")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ compiler flags: ${CMAKE_CXX_FLAGS}")