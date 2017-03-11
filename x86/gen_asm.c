#include <x86/gen_asm.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

bool generate_expression(statement_expression_t * expression, closure_t * closure);

variable_t * get_variable(closure_t * closure, char * identifier) {
    variable_t * current_variable = closure->variables;
    while (current_variable != NULL) {
        if (0 == strcmp(current_variable->variable_name, identifier)) {
            return current_variable;
        }
        current_variable = current_variable->next;
    }

    /* TODO: handle error */
    return NULL;
}

variable_t * allocate_variable(closure_t * closure, size_t size, char * identifier) {
    variable_t * new_variable = malloc(sizeof(*new_variable));
    if (NULL == new_variable) {
        return NULL;
    }
    new_variable->next = closure->variables;
    new_variable->type = VALUE_TYPE_VARIABLE;
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

bool load_from_stack(variable_t * variable, closure_t * closure, register_e target_register) {
    asm_node_t * node = malloc(sizeof(*node));
    if (NULL == node) {
        return false;
    }

    node->operand1.type = OPERAND_TYPE_REG;
    node->operand1.reg = target_register;

    switch (variable->size) {
        case 4: /* TODO: YAY INT ONLY */
            node->operand2.type = OPERAND_TYPE_STACK_DWORD;
        default:
            return false;
    }
    node->operand2.stack_offset = variable->position.stack_offset;

    add_instruction_to_closure(node, closure);
    return true;
}

bool load_const_to_register(long constant, closure_t * closure, register_e target_register) {
    asm_node_t * node = malloc(sizeof(*node));
    if (NULL == node) {
        return false;
    }

    node->operand1.type = OPERAND_TYPE_REG;
    node->operand1.reg = target_register;
    node->operand2.type = OPERAND_TYPE_SIGNED_DWORD_CONST;
    node->operand2.signed_dword = (int32_t)constant;

    add_instruction_to_closure(node, closure);
    return true;
}

bool load_expression_to_register(statement_expression_t * expression, closure_t * closure, register_e target_register) {
    variable_t * expression_result = NULL;
    if (expression->type == EXPRESSION_TYPE_OP) {
        if(!generate_expression(expression, closure)) {
            return false;
        }
    }
    expression_result = lookup_expression_result(expression, closure);
    if (NULL == expression_result) {
        return false;
    }

    switch (expression->type) {
        case EXPRESSION_TYPE_CONST:
            return load_const_to_register(expression->constant, closure, target_register);
        case EXPRESSION_TYPE_IDENTIFIER:
            /* TODO: error handling */
            return load_from_stack(get_variable(closure, expression->identifier), closure, target_register);
        /* if we got here, our value is on the stack */
        default:
            return load_from_stack(expression_result, closure, target_register);
    }
}

variable_t * allocate_variable_from_destination(statement_expression_t * dst_expression, closure_t * closure) {
    if (dst_expression->type == EXPRESSION_TYPE_IDENTIFIER) {
        return get_variable(closure, dst_expression->identifier);
    }

    /* TODO: recursively check for type of expression */
    return allocate_variable(closure, 4, "");
}

bool generate_addition(statement_expression_t * expression, closure_t * closure) {
    bool success = false;
    variable_t * result = NULL;
    expression_op_t * addition = &expression->exp_op;
    asm_node_t *node_add = NULL;
    asm_node_t *node_store = NULL;

    node_add =  malloc(sizeof(*node_add));
    node_store = malloc(sizeof(*node_store));
    if (NULL == node_add || NULL == node_store) {
        success = false;
        goto cleanup;
    }

    if (!load_expression_to_register(addition->exp1, closure, REGISTER_EAX)) {
        success = false;
        goto cleanup;
    }

    if (!load_expression_to_register(addition->exp2, closure, REGISTER_EBX)) {
        success = false;
        goto cleanup;
    }

    node_add->opcode = OPCODE_ADD;
    node_add->operand1.type = OPERAND_TYPE_REG;
    node_add->operand1.reg = REGISTER_EAX;
    node_add->operand2.type = OPERAND_TYPE_REG;
    node_add->operand2.reg = REGISTER_EBX;

    result = allocate_variable_from_destination(addition->exp1, closure);
    if (NULL == result) {
        success = false;
        goto cleanup;
    }

    node_store->operand1.type = OPERAND_TYPE_STACK_DWORD;
    node_store->operand1.stack_offset = result->position.stack_offset;
    node_store->operand2.type = OPERAND_TYPE_REG;
    node_store->operand2.reg = REGISTER_EAX;

    add_instruction_to_closure(node_add, closure);
    add_instruction_to_closure(node_store, closure);

    success = true;

cleanup:
    if (!success) {
        if (node_add != NULL) {
            free(node_add);
        }
        if (node_store != NULL) {
            free(node_store);
        }
    }
    return success;
}

bool generate_assignment(statement_expression_t * expression, closure_t * closure) {
    expression_op_t * assignment = &expression->exp_op;
    /* TODO: error handling when left side is not variable */
    variable_t * assigned_variable = get_variable(closure, assignment->exp1->identifier);
    asm_node_t * node = malloc(sizeof(*node));
    if (NULL == node || assigned_variable == NULL) {
        return false;
    }

    memset(node, 0, sizeof(*node));
    if (assignment->exp1->type == EXPRESSION_TYPE_IDENTIFIER && assignment->exp2->type == EXPRESSION_TYPE_CONST) {
        node->next = NULL;
        node->opcode = OPCODE_MOV;
        node->operand1.type = OPERAND_TYPE_STACK_DWORD;
        node->operand1.stack_offset = assigned_variable->position.stack_offset;
        node->operand2.type = OPERAND_TYPE_SIGNED_DWORD_CONST;
        /* TODO: add overflow checks */
        node->operand2.signed_dword = (int32_t)assignment->exp2->constant;

        add_instruction_to_closure(node, closure);
        assigned_variable->evaluated_expression = expression;
    } else {
        return false;
    }

    return true;
}

bool generate_operation(statement_expression_t * expression, closure_t * closure) {
    switch (expression->exp_op.op) {
        case OP_ASSIGN:
            return generate_assignment(expression, closure);
        case OP_ADD:
            return generate_addition(expression, closure);
        default:
            return false;
    }
}

bool generate_expression(statement_expression_t * expression, closure_t * closure) {
    switch (expression->type) {
        case EXPRESSION_TYPE_CONST:
            /* well, the fuck we should do here */
            break;
        case EXPRESSION_TYPE_IDENTIFIER:
            /* same with const.. */
            break;
        case EXPRESSION_TYPE_OP:
            if(!generate_operation(expression, closure)) {
                return false;
            }
            break;
    }

    return true;
}

bool generate_declaration(statement_t * statement, closure_t * closure) {
    /* TODO: check type of declaration */
    if (NULL == allocate_variable(closure, 4, statement->declaration.identifier)) {
        return false;
    }

    return true;
}

bool parse_block(code_block_t * code_block, closure_t * closure)
{
    statement_t * current_statement = code_block->first_line;
    while (current_statement != NULL) {
        switch (current_statement->statement_type) {
            case STATEMENT_TYPE_DECLARATION:
                if (!generate_declaration(current_statement, closure)) {
                    return false;
                }
                break;

            case STATEMENT_TYPE_EXPRESSION:
                if (!generate_expression(&current_statement->expression, closure)) {
                    return false;
                }
                break;

            case STATEMENT_TYPE_IFELSE:
                return false; // not supported yet (never)
        }
        current_statement = current_statement->next;
    }

    return true;
}

static char * instruction_to_text[] = {
    "mov",
    "add",
    "sub",
    "mul",
    "div",
    "jmp"
};

static char * register_to_text[] = {
    "eax",
    "ebx",
    "ecx",
    "edx",
    "edi",
    "esi"
};

/* TODO: add error handling in this file */

bool print_operand(operand_e * operand, char * instruction_text, size_t instruction_text_size) {
    char operand_text[256] = {0};
    switch(operand->type) {
        case OPERAND_TYPE_STACK_DWORD:
            if (operand->type == OPERAND_TYPE_STACK_DWORD) {
                strncat(instruction_text, "dword ptr [ebp", instruction_text_size);
            }
        case OPERAND_TYPE_STACK_WORD:
            if (operand->type == OPERAND_TYPE_STACK_WORD) {
                strncat(instruction_text, "word ptr [ebp", instruction_text_size);
            }
        case OPERAND_TYPE_STACK_BYTE:
            if (operand->type == OPERAND_TYPE_STACK_BYTE) {
                strncat(instruction_text, "byte ptr [ebp", instruction_text_size);
            }
            if (operand->stack_offset < 0) {
                snprintf(operand_text, sizeof(operand_text), "-0x%lx", -operand->stack_offset);
            }
            else {
                snprintf(operand_text, sizeof(operand_text), "+0x%lx", operand->stack_offset);
            }
            strncat(instruction_text, operand_text, instruction_text_size);
            strncat(instruction_text, "]", instruction_text_size);
            break;

        case OPERAND_TYPE_SIGNED_DWORD_CONST:
            snprintf(operand_text, sizeof(operand_text), "%d", operand->signed_dword);
            strncat(instruction_text, operand_text, instruction_text_size);
            break;
        case OPERAND_TYPE_UNSIGNED_DWORD_CONST:
            snprintf(operand_text, sizeof(operand_text), "%ud", operand->unsigned_dword);
            strncat(instruction_text, operand_text, instruction_text_size);
            break;
        case OPERAND_TYPE_SIGNED_WORD_CONST:
            snprintf(operand_text, sizeof(operand_text), "%d", operand->signed_word);
            strncat(instruction_text, operand_text, instruction_text_size);
            break;
        case OPERAND_TYPE_UNSIGNED_WORD_CONST:
            snprintf(operand_text, sizeof(operand_text), "%ud", operand->unsigned_word);
            strncat(instruction_text, operand_text, instruction_text_size);
            break;
        case OPERAND_TYPE_SIGNED_BYTE_CONST:
            snprintf(operand_text, sizeof(operand_text), "%d", operand->signed_byte);
            strncat(instruction_text, operand_text, instruction_text_size);
            break;
        case OPERAND_TYPE_UNSIGNED_BYTE_CONST:
            snprintf(operand_text, sizeof(operand_text), "%ud", operand->unsigned_byte);
            strncat(instruction_text, operand_text, instruction_text_size);
            break;
        case OPERAND_TYPE_REG:
            snprintf(operand_text, sizeof(operand_text), "%s", register_to_text[operand->reg]);
            strncat(instruction_text, operand_text, instruction_text_size);
            break;
        default:
            return false;
    }
}

bool generate_assembly_instruction(asm_node_t * instruction, char * instruction_text, size_t instruction_text_size) {
    snprintf(instruction_text, instruction_text_size, "%s ", instruction_to_text[instruction->opcode]);
    if (instruction->operand1.type != OPERAND_TYPE_NONE) {
        print_operand(&instruction->operand1, instruction_text, instruction_text_size);
        if (instruction->operand2.type != OPERAND_TYPE_NONE) {
            strncat(instruction_text, ", ", instruction_text_size);
            print_operand(&instruction->operand2, instruction_text, instruction_text_size);
        }
    }
    return true;
}

bool generate_assembly(asm_node_t * instructions, int out_fd) {
    /* TODO: write to file, nigga */
    char * current_instruction_text = malloc(sizeof(*current_instruction_text) * 256);
    asm_node_t * current_instruction = instructions;

    if (NULL == current_instruction_text) {
        return false;
    }

    while (current_instruction != NULL) {
        memset(current_instruction_text, 0, sizeof(*current_instruction_text) * 256);
        if(!generate_assembly_instruction(
            current_instruction,
            current_instruction_text,
            sizeof(*current_instruction_text) * 256
        )) {
            return false;
        }
        write(out_fd, current_instruction_text, strlen(current_instruction_text));
        write(out_fd, "\n", 1);
        current_instruction = current_instruction->next;
    }
    return false;
}

bool gen_asm_x86(code_file_t * code_file, int out_fd)
{
	closure_t file_closure = {
        .instructions = NULL,
        .variables = NULL
	};
	code_block_t * code_block = code_file->first_block;
	while (code_block != NULL)
	{
		parse_block(code_block, &file_closure);
		code_block = NULL; // TODO add the other blocks
	}

    return generate_assembly(file_closure.instructions, out_fd);
}
