#ifndef __AST_H__
#define __AST_H__

#include "ast_nodes.h"

typedef enum statement_type
{
	DECLARATION,
	EXPRESSION
} statement_type_e;

typedef struct statement
{
	struct statement * next;
	statement_type_e statement_type;
	union
	{
		statement_declaration_t declaration;
		statement_expression_t expression;
	};
} statement_t;

typedef struct code_file
{
	statement_t * first_line;
} code_file_t;

void add_statement(code_file_t * file, statement_t * statement);
code_file_t new_code_file();
void debug_ast(code_file_t * code_file);

#endif
