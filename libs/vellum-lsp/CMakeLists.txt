add_library(vellum_lsp STATIC
    ast_locator.cpp
    completion_collectors.cpp
    completion_context.cpp
    completion_items.cpp
    completions_provider.cpp
    declaration_index.cpp
    definitions_provider.cpp
    diagnostics.cpp
    document_analyzer.cpp
    document_store.cpp
    lsp_utf16.cpp
    lsp_locations.cpp
    navigation_utils.cpp
    semantic_tokens.cpp
    server.cpp
    static_analyze.cpp
)

target_compile_features(vellum_lsp PUBLIC cxx_std_20)

target_link_libraries(vellum_lsp PUBLIC lsp vellum_compiler)

target_include_directories(vellum_lsp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/libs/vellum-compiler
)
