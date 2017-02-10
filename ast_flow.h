#ifndef __AST_FLOW_H__
#define __AST_FLOW_H__

typedef struct statement_ifelse statement_ifelse_t;

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

#endif
