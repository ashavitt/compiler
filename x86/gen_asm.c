#include <x86/gen_asm.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <x86/closure.h>

bool generate_expression(statement_expression_t * expression, closure_t * closure);

bool store_register_to_variable(variable_t * variable, register_e source_register, closure_t * closure) {
    asm_node_t * node_store = malloc(sizeof(*node_store));

    if (NULL == node_store) {
        return false;
    }

    node_store->opcode = OPCODE_MOV;
    node_store->operand1.type = OPERAND_TYPE_STACK_DWORD;
    node_store->operand1.stack_offset = variable->position.stack_offset;
    node_store->operand2.type = OPERAND_TYPE_REG;
    node_store->operand2.reg = source_register;
    add_instruction_to_closure(node_store, closure);
    return true;
}

bool load_from_stack(variable_t * variable, closure_t * closure, register_e target_register) {
	asm_node_t * node = NULL;

	if (NULL == variable) {
		return false;
	}

	node = malloc(sizeof(*node));
	if (NULL == node) {
		return false;
	}

	node->operand1.type = OPERAND_TYPE_REG;
	node->operand1.reg = target_register;

	switch (variable->size) {
		case 4: /* TODO: YAY INT ONLY */
			node->operand2.type = OPERAND_TYPE_STACK_DWORD;
			break;
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

	switch (expression->type) {
		case EXPRESSION_TYPE_CONST:
			return load_const_to_register(expression->constant, closure, target_register);
		case EXPRESSION_TYPE_IDENTIFIER:
			/* TODO: error handling */
			return load_from_stack(get_variable(closure, expression->identifier), closure, target_register);
		/* if we got here, our value is on the stack */
		default:
			expression_result = lookup_expression_result(expression, closure);
			if (NULL == expression_result) {
				return false;
			}
			return load_from_stack(expression_result, closure, target_register);
	}
}

variable_t * allocate_variable_from_destination(statement_expression_t * dst_expression, closure_t * closure) {
	if (dst_expression->type == EXPRESSION_TYPE_IDENTIFIER) {
		return get_variable(closure, dst_expression->identifier);
	}

	/* TODO: recursively check for type of expression */
	return allocate_variable(closure, 4, "", VALUE_TYPE_EXPRESSION_RESULT);
}

bool generate_assignment(statement_expression_t * expression, closure_t * closure) {
	expression_op_t * assignment = &expression->exp_op;

	/* we are yet to handle non-variable lvalues */
	if (assignment->exp1->type != EXPRESSION_TYPE_IDENTIFIER) {
		return false;
	}

	variable_t * assigned_variable = get_variable(closure, assignment->exp1->identifier);
	if (NULL == assigned_variable) {
		return false;
	}

	asm_node_t * node = malloc(sizeof(*node));
	if (NULL == node) {
		return false;
	}

	memset(node, 0, sizeof(*node));
	if (assignment->exp1->type == EXPRESSION_TYPE_IDENTIFIER) {
		if(!generate_expression(assignment->exp2, closure)) {
			return false;
		}

		if (!load_expression_to_register(assignment->exp2, closure, REGISTER_EAX)) {
			return false;
		}

		node->opcode = OPCODE_MOV;
		node->operand1.type = OPERAND_TYPE_STACK_DWORD;
		node->operand1.stack_offset = assigned_variable->position.stack_offset;
		node->operand2.type = OPERAND_TYPE_REG;
		node->operand2.reg = REGISTER_EAX;

		add_instruction_to_closure(node, closure);
		assigned_variable->evaluated_expression = expression;
	} else {
		return false;
	}

	return true;
}



bool generate_arithmetic_operator(statement_expression_t * expression, closure_t * closure, opcode_e opcode) {
	bool success = false;
	variable_t * result = NULL;
	expression_op_t * addition = &expression->exp_op;
	asm_node_t *node_op = NULL;

	node_op =  malloc(sizeof(*node_op));
	if (NULL == node_op) {
		success = false;
		goto cleanup;
	}

	if(!generate_expression(addition->exp1, closure)) {
		return false;
	}

	if(!generate_expression(addition->exp2, closure)) {
		return false;
	}

	if (!load_expression_to_register(addition->exp1, closure, REGISTER_EAX)) {
		success = false;
		goto cleanup;
	}

	if (!load_expression_to_register(addition->exp2, closure, REGISTER_EBX)) {
		success = false;
		goto cleanup;
	}

	node_op->operand1.type = OPERAND_TYPE_REG;
	node_op->operand1.reg = REGISTER_EAX;
	node_op->operand2.type = OPERAND_TYPE_REG;
	node_op->operand2.reg = REGISTER_EBX;
	node_op->opcode = opcode;

	result = allocate_variable_from_destination(addition->exp1, closure);
	if (NULL == result) {
		success = false;
		goto cleanup;
	}

    if (!store_register_to_variable(result, REGISTER_EAX, closure)) {
        success = false;
        goto cleanup;
    }

	add_instruction_to_closure(node_op, closure);
	result->evaluated_expression = expression;

	success = true;

cleanup:
	if (!success) {
		if (node_op != NULL) {
			free(node_op);
		}
	}
	return success;
}

bool generate_unary_operator(statement_expression_t * expression, closure_t * closure, opcode_e opcode) {
    bool success = false;
    asm_node_t * opcode_asm = malloc(sizeof(*opcode_asm));

    if (NULL == opcode_asm) {
        goto cleanup;
    }

    if(!generate_expression(expression->exp_op.exp1, closure)) {
        goto cleanup;
    }

    if (!load_expression_to_register(expression->exp_op.exp1, closure, REGISTER_EAX))
    {
        goto cleanup;
    }

    /* TODO: fix when there are types */
    variable_t * result =  allocate_variable(closure, 4, "", VALUE_TYPE_EXPRESSION_RESULT);

    opcode_asm->opcode = opcode;
    opcode_asm->operand1.type = OPERAND_TYPE_REG;
    opcode_asm->operand1.reg = REGISTER_EAX;
    opcode_asm->operand2.type = OPERAND_TYPE_NONE;
    add_instruction_to_closure(opcode_asm, closure);
    opcode_asm = NULL;

    if (!store_register_to_variable(result, REGISTER_EAX, closure)) {
        goto cleanup;
    }

    result->evaluated_expression = expression;
    success = true;

cleanup:
    if (NULL != opcode_asm) {
        free(opcode_asm);
    }
    return success;
}

bool generate_operation(statement_expression_t * expression, closure_t * closure) {
	switch (expression->exp_op.op) {
		case OP_ASSIGN:
			return generate_assignment(expression, closure);
		case OP_ADD:
			return generate_arithmetic_operator(expression, closure, OPCODE_ADD);
		case OP_BAND:
			return generate_arithmetic_operator(expression, closure, OPCODE_AND);
		case OP_BOR:
			return generate_arithmetic_operator(expression, closure, OPCODE_OR);
		case OP_SUB:
			return generate_arithmetic_operator(expression, closure, OPCODE_SUB);
        case OP_NEG:
            return generate_unary_operator(expression, closure, OPCODE_NEG);
        case OP_PLUS:
            /* basically just transfer expression result to the current one */
            if (!generate_expression(expression->exp_op.exp1, closure)) {
                return false;
            }
            variable_t * result = lookup_expression_result(expression->exp_op.exp1, closure);
            if (result == NULL) {
                return false;
            }
            result->evaluated_expression = expression;
            return true;
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
	if (NULL == allocate_variable(closure, 4, statement->declaration.identifier, VALUE_TYPE_VARIABLE)) {
		return false;
	}

	return true;
}

bool generate_ifelse(statement_ifelse_t * ifelse, closure_t * closure)
{
	variable_t * if_expr_variable = NULL;
	asm_node_t * check_evaluated_expression = NULL;
	asm_node_t * jmp_over_if = NULL;
	asm_node_t * jmp_over_else = NULL;
	asm_node_t * after_if_opcode = NULL;
	asm_node_t * after_else_opcode = NULL;

	check_evaluated_expression = malloc(sizeof(*check_evaluated_expression));
	jmp_over_if = malloc(sizeof(*jmp_over_if));

    if(!generate_expression(ifelse->if_expr, closure)) {
        return false;
    }

	if (!load_expression_to_register(ifelse->if_expr, closure, REGISTER_EAX))
	{
		return false;
	}

	/* load the evaluated expression and check if its 0 */
	check_evaluated_expression->opcode = OPCODE_OR;
	check_evaluated_expression->operand1.type = OPERAND_TYPE_REG;
	check_evaluated_expression->operand1.reg = REGISTER_EAX;
	check_evaluated_expression->operand2.type = OPERAND_TYPE_REG;
	check_evaluated_expression->operand2.reg = REGISTER_EAX;
	add_instruction_to_closure(check_evaluated_expression, closure);

	jmp_over_if->opcode = OPCODE_JZ;
	jmp_over_if->operand1.type = OPERAND_TYPE_REFERENCE;
	jmp_over_if->operand1.ref = NULL; /* this will be set later on */
	add_instruction_to_closure(jmp_over_if, closure);

	if (!parse_block(ifelse->if_block, closure))
	{
		return false;
	}

	if (NULL == ifelse->else_block)
	{
		after_if_opcode = malloc(sizeof(*after_if_opcode));
		after_if_opcode->opcode = OPCODE_NOP;
		add_instruction_to_closure(after_if_opcode, closure);
	}
	else
	{
		/* add a jmp so the else block won't be executed after the if block */
		jmp_over_else = malloc(sizeof(*jmp_over_else));
		jmp_over_else->opcode = OPCODE_JMP;
		jmp_over_else->operand1.type = OPERAND_TYPE_REFERENCE;
		jmp_over_else->operand1.ref = NULL; /* this will be set later */
		add_instruction_to_closure(jmp_over_else, closure);
		
		/* add the else block */
		if (!parse_block(ifelse->else_block, closure))
		{
			return false;
		}

		/* set the start of the else block */
		/* TODO what if the else block is empty but still exists?? */
		after_if_opcode = jmp_over_else->next;

		after_else_opcode = malloc(sizeof(*after_else_opcode));
		after_else_opcode->opcode = OPCODE_NOP;
		add_instruction_to_closure(after_else_opcode, closure);
		jmp_over_else->operand1.ref = after_else_opcode;
		add_label_to_node(after_else_opcode, closure);
	}
	/* This is a little hack, set the jmp destination if the condition is not met */
	jmp_over_if->operand1.ref = after_if_opcode;
	add_label_to_node(after_if_opcode, closure);
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
				if (!generate_ifelse(&current_statement->ifelse, closure))
				{
					return false;
				}
				break;
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
	"imul",
	"div",
	"xor",
	"and",
	"or",
	"jmp",
	"push",
	"pop",
	"jz",
	"jnz",
	"nop",
    "neg"
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

bool print_operand(operand_e * operand, char * instruction_text, size_t instruction_text_size, closure_t * closure) {
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
		case OPERAND_TYPE_REFERENCE:
			snprintf(operand_text, sizeof(operand_text), "%s%lu", closure->closure_name, operand->ref->label_index);
			strncat(instruction_text, operand_text, instruction_text_size);
			break;
		default:
			return false;
	}
}

bool generate_assembly_instruction(asm_node_t * instruction, char * instruction_text, size_t instruction_text_size, closure_t * closure) {
	if (instruction->label_index != 0) {
		snprintf(
				instruction_text,
				instruction_text_size,
				"%s_%lu: %s ",
				closure->closure_name,
				instruction->label_index, instruction_to_text[instruction->opcode]
		);
	}
	else {
		snprintf(instruction_text, instruction_text_size, "%s ", instruction_to_text[instruction->opcode]);
	}
	if (instruction->operand1.type != OPERAND_TYPE_NONE) {
		print_operand(&instruction->operand1, instruction_text, instruction_text_size, closure);
		if (instruction->operand2.type != OPERAND_TYPE_NONE) {
			strncat(instruction_text, ", ", instruction_text_size);
			print_operand(&instruction->operand2, instruction_text, instruction_text_size, closure);
		}
	}
	return true;
}

bool generate_assembly(asm_node_t * instructions, int out_fd, closure_t * closure) {
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
			sizeof(*current_instruction_text) * 256,
			closure
		)) {
			return false;
		}
		write(out_fd, current_instruction_text, strlen(current_instruction_text));
		write(out_fd, "\n", 1);
		current_instruction = current_instruction->next;
	}
	return true;
}

bool gen_asm_x86(code_file_t * code_file, int out_fd)
{
	closure_t file_closure = {
		.instructions = NULL,
		.variables = NULL,
		.label_count = 1,
		.closure_name = "global_closure"
	};
	code_block_t * code_block = code_file->first_block;
	while (code_block != NULL)
	{
		if(!parse_block(code_block, &file_closure)) {
			printf("Failed parsing to intermediate RISC\nGenerating assembly anyway.\n");
		}
		code_block = NULL; // TODO add the other blocks
	}
	return generate_assembly(file_closure.instructions, out_fd, &file_closure);
}
