#ifndef __GEN_ASM_H__
#define __GEN_ASM_H__

#include <ast.h>
#include <stdbool.h>
#include <x86/closure.h>
#include <functions.h>

typedef struct code_file_data code_file_data_t;

bool parse_block(code_block_t * code_block, closure_t * closure, type_space_t *type_space);
bool gen_asm_x86(function_node_t * code_file, int out_fd);
bool initialize_type_generation(type_t * added_type);
bool generate_expression(statement_expression_t * expression, closure_t * closure, type_space_t *type_space);
#endif
