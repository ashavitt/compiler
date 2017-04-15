#ifndef TYPE_CHECK_H
#define TYPE_CHECK_H

#include <types/types.h>

bool type_check_expression(
	type_space_t *type_space,
	statement_expression_t *expression,
	closure_t *closure
);

bool type_check_block(type_space_t *type_space, code_block_t *code_block, closure_t *closure);

bool is_lvalue(
	type_space_t *type_space,
	statement_expression_t *expression
);

#endif