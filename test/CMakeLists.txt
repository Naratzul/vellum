find_package(Catch2 REQUIRED)

add_executable(tests
    unit/lexer_tests.cpp
)

target_link_libraries(tests PRIVATE
    vellum_compiler
    Catch2::Catch2WithMain
)

add_test(NAME VellumTests COMMAND tests)