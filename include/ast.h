#ifndef __AST_H__
#define __AST_H__

typedef struct code_block code_block_t;

#include "ast_nodes.h"
#include "ast_flow.h"

typedef enum statement_type
{
	DECLARATION,
	EXPRESSION,
	IFELSE
} statement_type_e;

typedef struct statement
{
	struct statement * next;
	statement_type_e statement_type;
	union
	{
		statement_declaration_t declaration;
		statement_expression_t expression;
		statement_ifelse_t ifelse;
	};
} statement_t;

struct code_block
{
	statement_t * first_line;
};

typedef struct code_file
{
	code_block_t * first_block;
} code_file_t;

statement_t * create_statement_expression(statement_expression_t * expr);
statement_t * create_statement_declaration(statement_declaration_t * decl);
statement_t * create_statement_ifelse(statement_ifelse_t * ifelse);
void add_statement(code_block_t * file, statement_t * statement);
void debug_ast(code_file_t * code_file);
void debug_code_block(code_block_t * code_block, int offset);

#endif