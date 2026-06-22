include(FetchContent)
FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v3.3.1
)
FetchContent_MakeAvailable(cxxopts)

add_executable(vellum main.cpp)
target_link_libraries(vellum PRIVATE vellum_compiler vellum_diag)
target_include_directories(vellum PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/vellum-compiler
    ${CMAKE_SOURCE_DIR}/libs/vellum-diag
    ${cxxopts_SOURCE_DIR}/include)
target_compile_definitions(vellum PRIVATE
    "VELLUM_VERSION=\"${PROJECT_VERSION}\""
    "VELLUM_SENTRY_RELEASE=\"vellum@${PROJECT_VERSION}\"")
if(DEFINED VELLUM_SENTRY_DSN AND NOT VELLUM_SENTRY_DSN STREQUAL "")
  target_compile_definitions(vellum PRIVATE
    "VELLUM_SENTRY_DSN=\"${VELLUM_SENTRY_DSN}\"")
endif()
if(DEFINED VELLUM_SENTRY_ENVIRONMENT AND NOT VELLUM_SENTRY_ENVIRONMENT STREQUAL "")
  target_compile_definitions(vellum PRIVATE
    "VELLUM_SENTRY_ENVIRONMENT=\"${VELLUM_SENTRY_ENVIRONMENT}\"")
endif()
