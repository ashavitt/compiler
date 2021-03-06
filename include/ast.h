#ifndef __AST_H__
#define __AST_H__

typedef struct code_block code_block_t;

#include <ast_nodes.h>
#include <ast_flow.h>
#include <ast_functions.h>
#include <functions.h>

typedef enum statement_type
{
	STATEMENT_TYPE_DECLARATION,
	STATEMENT_TYPE_TYPE_DECLARATION,
	STATEMENT_TYPE_EXPRESSION,
	STATEMENT_TYPE_IFELSE,
	STATEMENT_TYPE_LOOP,
	STATEMENT_TYPE_BREAK,
	STATEMENT_TYPE_CALL_FUNCTION
} statement_type_e;

typedef struct statement
{
	struct statement * next;
	statement_type_e statement_type;
	union
	{
		statement_declaration_t declaration;
		statement_expression_t expression;
		statement_type_declaration_t type_declaration;
		statement_ifelse_t ifelse;
		statement_loop_t loop;
		statement_call_function_t call_function;
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
statement_t * create_statement_declaration(statement_declaration_t * decl, statement_expression_t * initial_value);
statement_t * create_statement_ifelse(statement_ifelse_t * ifelse);
statement_t * create_statement_loop(statement_loop_t * loop);
statement_t * create_statement_type_declaration(statement_type_declaration_t * decl);
statement_t * create_statement_break();
statement_t * create_statement_call_function(statement_call_function_t * call_function);
void add_statement(code_block_t * file, statement_t * statement);
void debug_ast(function_node_t * function_list);
void debug_code_block(code_block_t * code_block, int offset);

#endif
