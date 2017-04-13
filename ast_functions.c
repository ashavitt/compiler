#include <ast_functions.h>
#include <stdlib.h>

statement_call_function_t * create_call_function_statement(
	char * function_name,
	statement_call_function_param_t * params)
{
	statement_call_function_t * new_call_function = NULL;

	new_call_function = malloc(sizeof(*new_call_function));
	if (NULL == new_call_function)
	{
		goto cleanup;
	}

	new_call_function->function_name = function_name;
	new_call_function->params = params;
	return new_call_function;
cleanup:
	return NULL;
}

statement_call_function_param_t * add_call_function_parameter(
	statement_call_function_param_t * previous_parameter,
	statement_expression_t * parameter)
{
	statement_call_function_param_t * new_parameter = malloc(sizeof(*new_parameter));
	if (NULL == new_parameter)
	{
		goto cleanup;
	}

	new_parameter->next = previous_parameter;
	new_parameter->parameter = parameter;

	return new_parameter;

cleanup:
	/* TODO free the whole list */
	free(previous_parameter);
	return NULL;
}
