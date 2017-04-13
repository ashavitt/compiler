#include <functions.h>
#include <stdlib.h>
#include <string.h>

function_declaration_t * create_function_declaration(
	statement_declaration_t * return_value_type,
	function_parameter_t * parameter_list,
	code_block_t * function_code)
{
	function_declaration_t * new_function = malloc(sizeof(*new_function));
	if (NULL == new_function)
	{
		goto cleanup;
	}

	new_function->parameter_list = parameter_list;
	new_function->return_value_type = return_value_type->type;
	new_function->identifier = return_value_type->identifier;
	new_function->function_code = function_code;

	/* this is just a wrapper struct */
	free(return_value_type);

	return new_function;
cleanup:
	/* TODO recursive free */
	free(function_code);
	free(return_value_type);
	return NULL;
}

function_parameter_t * add_function_parameter(
	function_parameter_t * previous_parameter,
	statement_declaration_t * parameter_type)
{
	function_parameter_t * new_parameter = malloc(sizeof(*new_parameter));
	if (NULL == new_parameter)
	{
		goto cleanup;
	}

	new_parameter->next = previous_parameter;
	new_parameter->parameter_type = parameter_type->type;
	new_parameter->parameter_identifier = parameter_type->identifier;

	free(parameter_type);
	return new_parameter;

cleanup:
	free(parameter_type);
	/* TODO free the whole list */
	free(previous_parameter);
	return NULL;
}

void register_new_function(
	function_declaration_t * function_declaration,
	function_node_t * function_list)
{
	function_node_t * last_function = function_list;
	function_node_t * new_function = malloc(sizeof(*new_function));
	if (NULL == new_function)
	{
		return;
	}
	new_function->next = NULL;
	new_function->function = function_declaration;

	while (last_function->next != NULL)
	{
		last_function = last_function->next;
	}

	last_function->next = new_function;
}

function_declaration_t * lookup_function(
	char * function_name,
	function_node_t * function_list)
{
	char * temp_function_name = NULL;
	function_node_t * temp_function_node = function_list;

	while (temp_function_node != NULL)
	{
		temp_function_name = temp_function_node->function->identifier;
		if (0 == strcmp(temp_function_name, function_name))
		{
			return temp_function_node->function;
		}
		temp_function_node = temp_function_node->next;
	}
	return NULL;
}
