#include <ast.h>
#include <stdlib.h>

statement_ifelse_t * create_ifelse_statement(
	statement_expression_t * if_expr,
	code_block_t * if_block,
	code_block_t * else_block)
{
	statement_ifelse_t * new_ifelse = NULL;

	new_ifelse = (statement_ifelse_t *) malloc (sizeof(statement_ifelse_t));
	if (NULL == new_ifelse)
	{
		goto cleanup;
	}

	new_ifelse->if_expr = if_expr;
	new_ifelse->if_block = if_block;
	new_ifelse->else_block = else_block;

cleanup:
	return new_ifelse;
}
