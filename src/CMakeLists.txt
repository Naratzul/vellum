cmake_minimum_required (VERSION 3.12)

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-Wall)
if (MSVC)
    add_compile_options(/W0)
else()
    add_compile_options(-Wextra)
endif()

set(SOURCES 
    main.cpp 
    vellum.cpp
    
    ast/statement.cpp

    common/fs.cpp
    common/os.cpp
    
    compiler/compiler.cpp
    compiler/compiler_error_handler.cpp
    compiler/pex_object_compiler.cpp
    
    lexer/lexer.cpp
    
    parser/parser.cpp

    pex/pex_file.cpp
    pex/pex_object.cpp
    pex/pex_string_table.cpp
    pex/pex_user_flag.cpp
    )

add_executable (${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
