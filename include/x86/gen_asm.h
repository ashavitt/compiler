#ifndef __GEN_ASM_H__
#define __GEN_ASM_H__

#include <ast.h>
#include <stdbool.h>
#include <x86/closure.h>
#include <functions.h>

typedef struct code_file_data code_file_data_t;

typedef struct generation_operations_s {

	variable_t * (*allocate)(
		type_t * this,
		closure_t *closure,
		const char * identifier,
		value_type_e type
	);

	bool (*generate_operation)(
		expression_op_t * operation,
		closure_t * closure,
		type_space_t *type_space
	);

} generation_operations_t;

bool parse_block(code_block_t * code_block, closure_t * closure, type_space_t *type_space);
bool gen_asm_x86(function_node_t * code_file, int out_fd);

#endif
