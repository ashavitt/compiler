#include <functions.h>
#include <stdlib.h>
#include <string.h>

function_declaration_t * create_function_declaration(
	char * identifier,
	statement_declaration_t * return_value_type,
	function_parameter_t * parameter_list)
{
	char * identifier_copy = NULL;
	size_t identifier_len = 0;

	function_declaration_t * new_function = malloc(sizeof(*new_function));
	if (NULL == new_function)
	{
		goto cleanup;
	}

	identifier_len = strlen(identifier);
	identifier_copy = (char *) malloc (sizeof(char) * identifier_len);
	if (NULL == identifier_copy)
	{
		goto cleanup;
	}

	strncpy(identifier_copy, identifier, identifier_len);

	new_function->parameter_list = parameter_list;
	new_function->return_value_type = return_value_type->type;
	new_function->identifier = identifier_copy;

	/* this is just a wrapper struct */
	free(return_value_type);

	return new_function;
cleanup:
	free(identifier_copy);
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
