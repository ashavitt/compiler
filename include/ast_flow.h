#ifndef __AST_FLOW_H__
#define __AST_FLOW_H__

/* Structs and functions regarding control flow in C */

typedef struct statement_ifelse statement_ifelse_t;
typedef struct statement_loop statement_loop_t;

struct statement_ifelse
{
	statement_expression_t * if_expr;
	code_block_t * if_block;
	code_block_t * else_block;
};

statement_ifelse_t * create_ifelse_statement(
	statement_expression_t * if_expr,
	code_block_t * if_block,
	code_block_t * else_block
);

struct statement_loop
{
	statement_expression_t * init_expression;
	statement_expression_t * condition_expression;
	statement_expression_t * iteration_expression;
	code_block_t * loop_body;
};

statement_loop_t * create_loop_statement(
	statement_expression_t * init_expression,
	statement_expression_t * condition_expression,
	statement_expression_t * iteration_expression,
	code_block_t * loop_body
);

#endif
