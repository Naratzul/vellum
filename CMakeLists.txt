cmake_minimum_required(VERSION 3.12)
project(vellum VERSION 0.1.0 LANGUAGES CXX)

enable_testing()

include(FetchContent)
FetchContent_Declare(
  sentry-native
  GIT_REPOSITORY https://github.com/getsentry/sentry-native.git
  GIT_TAG        0.13.3
  GIT_SUBMODULES ""
)
set(SENTRY_BACKEND inproc CACHE STRING "" FORCE)
set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(sentry-native)

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