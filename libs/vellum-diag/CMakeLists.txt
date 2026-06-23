include(FetchContent)
set(CPPTRACE_BUILD_SHARED OFF CACHE BOOL "Build cpptrace as a shared library (OFF recommended for vellum)" FORCE)
FetchContent_Declare(
  cpptrace
  GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
  GIT_TAG        v1.0.4
)
FetchContent_MakeAvailable(cpptrace)

add_library(vellum_diag STATIC
    platform_paths.cpp
    sentry_report.cpp
    sentry_session.cpp
)

target_link_libraries(vellum_diag PUBLIC sentry cpptrace::cpptrace)
target_include_directories(vellum_diag PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(vellum_diag PUBLIC SENTRY_BUILD_STATIC=1)

include(${CMAKE_CURRENT_LIST_DIR}/vellum_sentry.cmake)
