cmake_minimum_required(VERSION 3.0)
project(compiler)

set(CMAKE_C_STANDARD 11)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lex ${CMAKE_BINARY_DIR}/yacc)

set(SOURCE_FILES
    ${CMAKE_BINARY_DIR}/yacc/y.tab.c
    ${CMAKE_BINARY_DIR}/lex/lex.yy.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ast.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ast_flow.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ast_nodes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol_table.c
    ${CMAKE_CURRENT_SOURCE_DIR}/types.c
    ${CMAKE_CURRENT_SOURCE_DIR}/x86/gen_asm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/x86/closure.c
)

include_directories(${CMAKE_BINARY_DIR}/lex ${CMAKE_BINARY_DIR}/yacc ${CMAKE_SOURCE_DIR}/include)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/lex/lex.yy.c
    COMMAND /usr/bin/lex ${CMAKE_CURRENT_SOURCE_DIR}/tokenizer.lex
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/lex/
)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/yacc/y.tab.c
    COMMAND /usr/bin/yacc -vd ${CMAKE_CURRENT_SOURCE_DIR}/parser.y -o ${CMAKE_BINARY_DIR}/yacc/y.tab.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(aramcc
    ${SOURCE_FILES}
)

target_link_libraries(aramcc fl)
