include(FetchContent)
FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v3.3.1
)
FetchContent_MakeAvailable(cxxopts)

set(CPPTRACE_BUILD_SHARED OFF CACHE BOOL "Build cpptrace as a shared library (OFF recommended for vellum)" FORCE)
FetchContent_Declare(
  cpptrace
  GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
  GIT_TAG        v1.0.4
)
FetchContent_MakeAvailable(cpptrace)

add_library(vellum_compiler STATIC
    analyze/declaration_collector.cpp
    analyze/import_library.cpp
    analyze/import_resolver.cpp
    analyze/semantic_analyzer.cpp
    analyze/type_checker.cpp
    analyze/type_collector.cpp

    ast/decl/declaration.cpp
    ast/expression/expression.cpp
    ast/statement/statement.cpp
    common/fs.cpp
    common/os.cpp
    common/string_set.cpp
    compiler/builtin_functions.cpp
    compiler/compiler.cpp
    compiler/compiler_error_handler.cpp
    compiler/pex_function_compiler.cpp
    compiler/pex_object_compiler.cpp
    compiler/resolver.cpp
    compiler/scope.cpp
    lexer/lexer.cpp
    parser/parser.cpp
    parser/papyrus_lexer.cpp
    parser/papyrus_parser.cpp
    pex/pex_debug_function_info.cpp
    pex/pex_debug_info.cpp
    pex/pex_file.cpp
    pex/pex_function_parameter.cpp
    pex/pex_function.cpp
    pex/pex_instruction.cpp
    pex/pex_object.cpp
    pex/pex_property.cpp
    pex/pex_state.cpp
    pex/pex_string_table.cpp
    pex/pex_user_flag.cpp
    pex/pex_value.cpp
    pex/pex_variable.cpp

    vellum/vellum_function.cpp
    vellum/vellum_identifier.cpp
    vellum/vellum_literal.cpp
    vellum/vellum_object.cpp
    vellum/vellum_property.cpp
    vellum/vellum_type.cpp
    vellum/vellum_value.cpp
    vellum/vellum_variable.cpp
    vellum/vellum.cpp
)

target_compile_options(vellum_compiler PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W0>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

target_include_directories(vellum_compiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(vellum main.cpp common/sentry_report.cpp)
target_link_libraries(vellum PRIVATE vellum_compiler sentry cpptrace::cpptrace)
target_include_directories(vellum PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                          ${cxxopts_SOURCE_DIR}/include)
target_compile_definitions(
  vellum
  PRIVATE SENTRY_BUILD_STATIC=1
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