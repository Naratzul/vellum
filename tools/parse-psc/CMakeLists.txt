add_executable(parse_psc parse_psc.cpp)
target_link_libraries(parse_psc PRIVATE vellum_compiler)
target_include_directories(parse_psc PRIVATE ${CMAKE_SOURCE_DIR}/libs/vellum-compiler)
