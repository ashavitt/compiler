#include <ast.h>
#include <ast_flow.h>
#include <stdlib.h>

statement_ifelse_t * create_ifelse_statement(
	statement_expression_t * if_expr,
	code_block_t * if_block,
	code_block_t * else_block)
{
	statement_ifelse_t * new_ifelse = NULL;

	new_ifelse = (statement_ifelse_t *) malloc (sizeof(*new_ifelse));
	if (NULL == new_ifelse)
	{
		/* TODO add recursive free */
		goto cleanup;
	}

	new_ifelse->if_expr = if_expr;
	new_ifelse->if_block = if_block;
	new_ifelse->else_block = else_block;

cleanup:
	return new_ifelse;
}

statement_loop_t * create_loop_statement(
	statement_expression_t * init_expression,
	statement_expression_t * condition_expression,
	statement_expression_t * iteration_expression,
	code_block_t * loop_body
) {
	statement_loop_t * new_loop = NULL;

	new_loop = (statement_loop_t *) malloc (sizeof(*new_loop));
	if (NULL == new_loop)
	{
		/* TODO add recursive free */
		goto cleanup;
	}

	new_loop->init_expression = init_expression;
	new_loop->condition_expression = condition_expression;
	new_loop->iteration_expression = iteration_expression;
	new_loop->loop_body = loop_body;

cleanup:
	return new_loop;
}
