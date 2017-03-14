#include <x86/closure.h>

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
