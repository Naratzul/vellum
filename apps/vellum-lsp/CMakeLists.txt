project(vellum-lsp VERSION 0.1.0 LANGUAGES CXX)

add_executable(vellum-lsp main.cpp)
target_link_libraries(vellum-lsp PRIVATE vellum_lsp)
vellum_target_sentry(vellum-lsp "vellum-lsp@${PROJECT_VERSION}")

set(VELLUM_LSP_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}")

install(TARGETS vellum-lsp
    RUNTIME DESTINATION ${VELLUM_LSP_OUTPUT_DIR}  # Windows
    BUNDLE DESTINATION ${VELLUM_LSP_OUTPUT_DIR}   # macOS
)

add_custom_command(TARGET vellum-lsp POST_BUILD
    COMMAND ${CMAKE_COMMAND} --install . --config $<CONFIG> --prefix "${CMAKE_SOURCE_DIR}"
)
