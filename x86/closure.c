#include <x86/closure.h>
#include <string.h>
#include <stdlib.h>

variable_t * lookup_expression_result(statement_expression_t * expression, closure_t * closure) {
    variable_t * current_variable = closure->variables;

    while (current_variable != NULL) {
        if (current_variable->evaluated_expression == expression) {
            return current_variable;
        }
        current_variable = current_variable->next;
    }

    return NULL;
}

variable_t * get_variable(closure_t * closure, char * identifier) {
    variable_t * current_variable = closure->variables;
    while (current_variable != NULL) {
        if (0 == strcmp(current_variable->variable_name, identifier)) {
            return current_variable;
        }
        current_variable = current_variable->next;
    }

    return NULL;
}

variable_t * allocate_variable(closure_t * closure, size_t size, char * identifier, value_type_e type) {
    variable_t * new_variable = malloc(sizeof(*new_variable));
    if (NULL == new_variable) {
        return NULL;
    }
    new_variable->next = closure->variables;
    new_variable->type = type;
    if (closure->variables == NULL) {
        new_variable->position = (position_t){
                .stack_offset = -4
        };
    } else {
        new_variable->position = (position_t){
                .stack_offset = closure->variables->position.stack_offset - 4
        };
    }
    new_variable->variable_name = identifier;
    new_variable->size = size;
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
