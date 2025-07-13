cmake_minimum_required(VERSION 3.10)
project(vellum-lsp)

set(CMAKE_CXX_STANDARD 20)

add_executable(vellum-lsp main.cpp "static_analyze.h" "static_analyze.cpp" "diagnostics.h" "diagnostics.cpp")
target_link_libraries(vellum-lsp PUBLIC lsp vellum_compiler)

target_include_directories(vellum-lsp PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

set(VELLUM_LSP_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}")

install(TARGETS vellum-lsp
    RUNTIME DESTINATION ${VELLUM_LSP_OUTPUT_DIR}  # Windows
    BUNDLE DESTINATION ${VELLUM_LSP_OUTPUT_DIR}   # macOS
)

add_custom_command(TARGET vellum-lsp POST_BUILD
    COMMAND ${CMAKE_COMMAND} --install . --config $<CONFIG> --prefix "${CMAKE_SOURCE_DIR}"
)