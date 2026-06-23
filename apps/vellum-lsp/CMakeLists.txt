cmake_minimum_required(VERSION 3.10)
project(vellum-lsp)

set(CMAKE_CXX_STANDARD 20)

add_executable(vellum-lsp
    main.cpp
    document_analyzer.h document_analyzer.cpp
    document_store.h document_store.cpp
    static_analyze.h static_analyze.cpp
    definitions_provider.h definitions_provider.cpp
    navigation_utils.h navigation_utils.cpp
    completion_context.h completion_context.cpp
    completion_collectors.h completion_collectors.cpp
    completion_items.h completion_items.cpp
    completions_provider.h completions_provider.cpp
    diagnostics.h diagnostics.cpp
    lsp_locations.h lsp_locations.cpp
    ast_locator.h ast_locator.cpp
    declaration_index.h declaration_index.cpp
    semantic_tokens.h semantic_tokens.cpp
     
    server.h server.cpp
)
target_link_libraries(vellum-lsp PRIVATE lsp vellum_compiler)
vellum_target_sentry(vellum-lsp "vellum-lsp@${PROJECT_VERSION}")

target_include_directories(vellum-lsp PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/vellum-compiler
)

set(VELLUM_LSP_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}")

install(TARGETS vellum-lsp
    RUNTIME DESTINATION ${VELLUM_LSP_OUTPUT_DIR}  # Windows
    BUNDLE DESTINATION ${VELLUM_LSP_OUTPUT_DIR}   # macOS
)

add_custom_command(TARGET vellum-lsp POST_BUILD
    COMMAND ${CMAKE_COMMAND} --install . --config $<CONFIG> --prefix "${CMAKE_SOURCE_DIR}"
)

add_executable(vellum-lsp-completion-smoke
    completion_smoke_test.cpp
    document_analyzer.cpp
    document_store.cpp
    static_analyze.cpp
    definitions_provider.cpp
    diagnostics.cpp
    lsp_locations.cpp
    ast_locator.cpp
    declaration_index.cpp
    semantic_tokens.cpp
    
    navigation_utils.cpp
    completion_context.cpp
    completion_collectors.cpp
    completion_items.cpp
    completions_provider.cpp
)
target_link_libraries(vellum-lsp-completion-smoke PUBLIC lsp vellum_compiler)
target_include_directories(vellum-lsp-completion-smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/vellum-compiler
)