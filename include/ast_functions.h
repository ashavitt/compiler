#ifndef __AST_FUNCTIONS_H__
#define __AST_FUNCTIONS_H__


typedef struct statement_call_function_s statement_call_function_t;
typedef struct statement_call_function_param_s statement_call_function_param_t;

#include <ast_nodes.h>

struct statement_call_function_param_s
{
	statement_expression_t * parameter;
	struct statement_call_function_param_s * next;
};

struct statement_call_function_s
{
	char * function_name;
	statement_call_function_param_t * params;
};

statement_call_function_t * create_call_function_statement(
	char * function_name,
	statement_call_function_param_t * params
);

statement_call_function_param_t * add_call_function_parameter(
	statement_call_function_param_t * previous_parameter,
	statement_expression_t * parameter
);

#endif
