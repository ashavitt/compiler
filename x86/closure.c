#include <x86/closure.h>
#include <string.h>
#include <stdlib.h>

variable_t * lookup_expression_result(
	statement_expression_t * expression,
	closure_t * closure,
	type_space_t *type_space
) {
	variable_t * current_variable = NULL;

	while (closure != NULL)
	{
		current_variable = closure->variables;

		while (current_variable != NULL) {
			if (current_variable->evaluated_expression == expression) {
				return current_variable;
			}
			current_variable = current_variable->next;
		}
		closure = closure->parent;
	}

	return NULL;
}

static variable_t * get_variable_from_closure(closure_t * closure, char * identifier) {
	variable_t * current_variable = NULL;
	current_variable = closure->parameters;
	while (current_variable != NULL) {
		if (0 == strcmp(current_variable->variable_name, identifier)) {
			return current_variable;
		}
		current_variable = current_variable->next;
	}

	current_variable = closure->variables;
	while (current_variable != NULL) {
		if (0 == strcmp(current_variable->variable_name, identifier)) {
			return current_variable;
		}
		current_variable = current_variable->next;
	}

	return NULL;
}

variable_t * get_variable(closure_t * closure, char * identifier)
{
	variable_t * current_variable = NULL;

	while (closure != NULL)
	{
		current_variable = get_variable_from_closure(closure, identifier);
		if (NULL != current_variable) {
			return current_variable;
		}
		/* go up in the closure hierarchy */
		closure = closure->parent;
	}

	return NULL;
}

static variable_t * allocate_parameter(
	closure_t * closure,
	char * identifier,
	value_type_e type,
	type_t *variable_type
) {
	closure_t * current_closure = closure;
	variable_t * new_variable = malloc(sizeof(*new_variable));
	if (NULL == new_variable) {
		return NULL;
	}

	/* named variable, check for name collision first */
	if ((strcmp(identifier, "") != 0) &&
		(NULL != get_variable_from_closure(closure, identifier))
	) {
		free(new_variable);
		return NULL;
	}

	new_variable->next = closure->parameters;
	new_variable->variable_type = type;

	/* check the previous closures */
	while (current_closure != NULL)
	{
		if (NULL != current_closure->parameters)
		{
			/* add the variable at the end of the stack */
			new_variable->position = (position_t){
				/* TODO: when structures and unions and such are passed, we should pass pointer instead */
				.stack_offset = current_closure->parameters->position.stack_offset + current_closure->parameters->type->size
			};
			break;
		}
		current_closure = current_closure->parent;
	}

	/* if no variable was found - this is the first */
	if (NULL == current_closure)
	{
		new_variable->position = (position_t){
			.stack_offset = 8
		};
	}

	new_variable->variable_name = identifier;
	new_variable->type = variable_type;
	closure->parameters = new_variable;

	return new_variable;
}

variable_t * allocate_variable(
	closure_t * closure,
	char * identifier,
	value_type_e type,
	type_t *variable_type
) {
	closure_t * current_closure = closure;
	variable_t * new_variable = NULL;

	/* make sure the collision check is made only on named variable */
	if (strcmp(identifier, "") != 0) {
		/* check that we don't allocate variable with the closure's name
		 * and check for name collision */
		if (
			(NULL != get_variable_from_closure(closure, identifier)) ||
			(0 == strcmp(identifier, closure->closure_name))
		) {
			free(new_variable);
			return NULL;
		}
	}

	if (type == VALUE_TYPE_PARAMETER) {
		return allocate_parameter(closure, identifier, type, variable_type);
	}

	new_variable = malloc(sizeof(*new_variable));
	if (NULL == new_variable) {
		return NULL;
	}

	new_variable->next = closure->variables;
    new_variable->variable_type = type;

	/* check the previous closures */
	while (current_closure != NULL)
	{
		if (NULL != current_closure->variables)
		{
			/* add the variable at the end of the stack */
			new_variable->position = (position_t){
				.stack_offset = current_closure->variables->position.stack_offset - variable_type->size
			};
			break;
		}
		current_closure = current_closure->parent;
	}

	/* if no variable was found - this is the first */
	if (NULL == current_closure)
	{
		new_variable->position = (position_t){
			.stack_offset = -variable_type->size
		};
	}

	new_variable->variable_name = identifier;
	new_variable->type = variable_type;
	closure->variables = new_variable;

	return new_variable;
}

void add_instruction_to_closure(asm_node_t * node, closure_t * closure) {
    asm_node_t * current_instruction = closure->instructions;

    if (current_instruction == NULL) {
        closure->instructions = node;
    } else {
        while (current_instruction->next != NULL) {
            current_instruction = current_instruction->next;
        }
        current_instruction->next = node;
    }
}

void add_label_to_node(
	asm_node_t * node,
	closure_t * closure)
{
	if (NULL == node)
	{
		return;
	}
	if (0 == node->label_index)
	{
		node->label_index = closure->label_count;
		closure->label_count = closure->label_count + 1;
	}
}

closure_t * enter_new_closure(
	closure_t * old_closure,
	const char * new_closure_name
) {
	closure_t * new_closure = malloc(sizeof(*new_closure));

	new_closure->parent = old_closure;
	new_closure->instructions = NULL;
	new_closure->variables = NULL;
	new_closure->parameters = NULL;
	if (old_closure != NULL) {
		new_closure->label_count = old_closure->label_count;
		if (new_closure_name == NULL) {
			new_closure->closure_name = old_closure->closure_name;
		}
		new_closure->break_to_instruction = old_closure->break_to_instruction;
	} else {
		new_closure->label_count = 1;
		new_closure->break_to_instruction = NULL;
	}

	if (new_closure_name != NULL) {
		new_closure->closure_name = new_closure_name;
	}

	return new_closure;
}

closure_t * exit_closure(
	closure_t * old_closure
) {
	closure_t * new_closure = old_closure->parent;

	/* concatenate instructions */
	add_instruction_to_closure(old_closure->instructions, new_closure);
	/* increase the label count */
	new_closure->label_count = old_closure->label_count;

	return new_closure;
}
