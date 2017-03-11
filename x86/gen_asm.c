#include <x86/gen_asm.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

long get_variable_stack_offset(closure_t * closure, char * identifier) {
    variable_t * current_variable = closure->variables;
    while (current_variable != NULL) {
        if (0 == strcmp(current_variable->variable_name, identifier)) {
            return current_variable->position.stack_offset;
        }
        current_variable = current_variable->next;
    }

    /* TODO: handle error */
    return 0;
}

bool generate_assignment(expression_op_t * assignment, closure_t * closure) {
    asm_node_t * current_instruction = closure->instructions;
    asm_node_t * node = malloc(sizeof(*node));
    if (NULL == node) {
        return false;
    }

    memset(node, 0, sizeof(*node));
    if (assignment->exp1->type == EXPRESSION_TYPE_IDENTIFIER && assignment->exp2->type == EXPRESSION_TYPE_CONST) {
        node->next = NULL;
        node->opcode = OPCODE_MOV;
        node->operand1.type = OPERAND_TYPE_STACK_DWORD;
        node->operand1.stack_offset = get_variable_stack_offset(closure, assignment->exp1->identifier);
        node->operand2.type = OPERAND_TYPE_SIGNED_DWORD_CONST;
        /* TODO: add overflow checks */
        node->operand2.signed_dword = (int32_t)assignment->exp2->constant;

        /* add the instruction to the end of the list */
        if (current_instruction == NULL) {
            closure->instructions = node;
        } else {
            while (current_instruction->next != NULL) {
                current_instruction = current_instruction->next;
            }
            current_instruction->next = node;
        }
    } else {
        return false;
    }

    return true;
}

bool generate_operation(statement_expression_t * expression, closure_t * closure) {
    switch (expression->exp_op.op) {
        case OP_ASSIGN:
            return generate_assignment(&expression->exp_op, closure);
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
            return generate_operation(expression, closure);
    }
    return true;
}

bool generate_declaration(statement_t * statement, closure_t * closure) {
    variable_t * new_variable = malloc(sizeof(*new_variable));
    if (NULL == new_variable) {
        return false;
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
    new_variable->variable_name = statement->declaration.identifier;
    closure->variables = new_variable;
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
    "mov"
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
        default:
            return false;
    }

    return true;
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
