include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
  FIND_PACKAGE_ARGS NAMES Catch2
)
FetchContent_MakeAvailable(Catch2)

add_executable(tests
    unit/lexer_tests.cpp
)

target_link_libraries(tests PRIVATE
    vellum_compiler
    Catch2::Catch2WithMain
)

target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME VellumTests COMMAND tests)