cmake_minimum_required(VERSION 3.12)

project(vellum)

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

add_subdirectory(libs/vellum-compiler)
add_subdirectory(libs/vellum-diag)
add_subdirectory(apps/vellum)
add_subdirectory(tools/parse-psc)

set(LSP_USE_SANITIZERS OFF CACHE BOOL
    "Use address and undefined behavior sanitizers for lsp-framework Debug builds"
    FORCE)
FetchContent_Declare(
  lsp-framework
  GIT_REPOSITORY https://github.com/leon-bckl/lsp-framework.git
  GIT_TAG        66e12ebcbcf87d1ff50d18e998efa35f130339b3
)
FetchContent_MakeAvailable(lsp-framework)
add_subdirectory(libs/vellum-lsp)
add_subdirectory(apps/vellum-lsp)
add_subdirectory(test)