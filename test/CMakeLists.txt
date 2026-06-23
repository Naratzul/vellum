include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
  FIND_PACKAGE_ARGS NAMES Catch2
)
FetchContent_MakeAvailable(Catch2)

add_executable(tests
    unit/mock/mock_lexer.cpp

    unit/compiler_tests.cpp
    unit/lexer_tests.cpp
    unit/parser_tests.cpp
    unit/papyrus_parser_tests.cpp
    unit/semantic_tests.cpp
    unit/utils.cpp

    unit/lsp/completion_tests.cpp
)

target_compile_definitions(tests
    INTERFACE VELLUM_TESTS
    PRIVATE VELLUM_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(tests PRIVATE
    vellum_compiler
    vellum_lsp
    Catch2::Catch2WithMain
)

target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/vellum-compiler
    ${CMAKE_SOURCE_DIR}/test/unit/lsp
)

add_test(NAME VellumTests COMMAND tests)
