#include <x86/gen_asm.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <x86/closure.h>
#include <types.h>
#include <type_check.h>
#include <x86/types/int.h>

bool generate_expression(statement_expression_t * expression, closure_t * closure, type_space_t *type_space);

static bool store_register_to_variable(variable_t * variable, register_e source_register, closure_t * closure) {
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

static bool load_from_stack(variable_t * variable, closure_t * closure, register_e target_register) {
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

	switch (variable->type->size) {
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

static bool load_const_to_register(long constant, closure_t * closure, register_e target_register) {
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

static bool load_expression_to_register(
	statement_expression_t * expression,
	closure_t * closure,
	register_e target_register,
	type_space_t *type_space
) {
	variable_t * expression_result = NULL;

	switch (expression->expression_type) {
		case EXPRESSION_TYPE_CONST:
			return load_const_to_register(expression->constant, closure, target_register);
		case EXPRESSION_TYPE_IDENTIFIER:
			/* TODO: error handling */
			return load_from_stack(get_variable(closure, expression->identifier), closure, target_register);
		/* if we got here, our value is on the stack */
		default:
			expression_result = lookup_expression_result(expression, closure, type_space);
			if (NULL == expression_result) {
				return false;
			}
			return load_from_stack(expression_result, closure, target_register);
	}
}

bool generate_expression(statement_expression_t * expression, closure_t * closure, type_space_t *type_space) {
	variable_t * result = NULL;
	switch (expression->expression_type) {
		case EXPRESSION_TYPE_CONST:
			/* well, the fuck we should do here */
			break;
		case EXPRESSION_TYPE_IDENTIFIER:
			/* same with const.. */
			break;
		case EXPRESSION_TYPE_OP:
			/* TODO: call type's operation */
			result  = expression->type->allocate_variable(
				expression->type,
				closure,
				NULL,
				VALUE_TYPE_EXPRESSION_RESULT
			);
			if (NULL == result) {
				return false;
			}
			return result->op->generate_operation(expression, closure, type_space);
	}

	return true;
}

static bool generate_declaration(statement_t * statement, closure_t * closure, type_space_t *type_space) {
	/* the type checker should have already done this */
	/* TODO: should we remove this check? */
	if (NULL == get_variable(closure, statement->declaration.identifier)) {
		return false;
	}

	return true;
}

static bool generate_ifelse(statement_ifelse_t * ifelse, closure_t * closure, type_space_t *type_space)
{
	asm_node_t * check_evaluated_expression = NULL;
	asm_node_t * jmp_over_if = NULL;
	asm_node_t * jmp_over_else = NULL;
	asm_node_t * after_if_opcode = NULL;
	asm_node_t * after_else_opcode = NULL;

	check_evaluated_expression = malloc(sizeof(*check_evaluated_expression));
	jmp_over_if = malloc(sizeof(*jmp_over_if));

	if (!generate_expression(ifelse->if_expr, closure, type_space)) {
		return false;
	}

	if (!load_expression_to_register(ifelse->if_expr, closure, REGISTER_EAX, type_space))
	{
		return false;
	}

	/* TODO: convert to bool or int operation */
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

	if (!parse_block(ifelse->if_block, closure, type_space))
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
		if (!parse_block(ifelse->else_block, closure, type_space))
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

bool generate_loop(statement_loop_t * loop, closure_t * closure, type_space_t *type_space)
{
	asm_node_t * check_evaluated_expression = NULL;
	asm_node_t * jmp_over_loop = NULL;
	asm_node_t * after_init = NULL;
	asm_node_t * jmp_to_start = NULL;
	asm_node_t * end_of_loop = NULL;
	closure_t * loop_closure = NULL;

	end_of_loop = malloc(sizeof(*end_of_loop));
	end_of_loop->opcode = OPCODE_NOP;

	/* enter the loop's closure */
	loop_closure = enter_new_closure(closure, NULL);
	/* set the end of the loop for breaks */
	loop_closure->break_to_instruction = end_of_loop;

	/* first the initialization of the loop */
	if (loop->init_expression != NULL)
	{
		if(!generate_expression(loop->init_expression, loop_closure, type_space))
		{
			return false;
		}
	}

	after_init = malloc(sizeof(*after_init));
	after_init->opcode = OPCODE_NOP;
	add_instruction_to_closure(after_init, loop_closure);

	/* the loop condition */
	if (loop->condition_expression != NULL)
	{
		if (!generate_expression(loop->condition_expression, loop_closure, type_space))
		{
			return false;
		}
		if (!load_expression_to_register(loop->condition_expression, loop_closure, REGISTER_EAX, type_space))
		{
			return false;
		}

		check_evaluated_expression = malloc(sizeof(*check_evaluated_expression));
		jmp_over_loop = malloc(sizeof(*jmp_over_loop));

		/* load the evaluated expression and check if its 0 */
		check_evaluated_expression->opcode = OPCODE_OR;
		check_evaluated_expression->operand1.type = OPERAND_TYPE_REG;
		check_evaluated_expression->operand1.reg = REGISTER_EAX;
		check_evaluated_expression->operand2.type = OPERAND_TYPE_REG;
		check_evaluated_expression->operand2.reg = REGISTER_EAX;
		add_instruction_to_closure(check_evaluated_expression, loop_closure);

		jmp_over_loop->opcode = OPCODE_JZ;
		jmp_over_loop->operand1.type = OPERAND_TYPE_REFERENCE;
		jmp_over_loop->operand1.ref = NULL; /* this will be set later on */
		add_instruction_to_closure(jmp_over_loop, loop_closure);
	}
	
	/* the body of the loop */
	if (!parse_block(loop->loop_body, loop_closure, type_space))
	{
		return false;
	}

	/* loop stepping expression */
	if (loop->iteration_expression != NULL)
	{
		if (!generate_expression(loop->iteration_expression, loop_closure, type_space))
		{
			return false;
		}
	}

	jmp_to_start = malloc(sizeof(*jmp_to_start));
	jmp_to_start->opcode = OPCODE_JMP;
	jmp_to_start->operand1.type = OPERAND_TYPE_REFERENCE;
	jmp_to_start->operand1.ref = after_init;
	add_label_to_node(after_init, loop_closure);
	add_instruction_to_closure(jmp_to_start, loop_closure);

	add_instruction_to_closure(end_of_loop, loop_closure);

	if (loop->condition_expression != NULL)
	{
		jmp_over_loop->operand1.ref = end_of_loop;
		add_label_to_node(end_of_loop, loop_closure);
	}

	exit_closure(loop_closure);

	return true;
}

bool generate_break(closure_t * closure, type_space_t *type_space)
{
	asm_node_t * break_jmp = NULL;
	if (NULL == closure->break_to_instruction)
	{
		/* nothing to break from */
		return false;
	}

	break_jmp = malloc(sizeof(*break_jmp));
	if (NULL == break_jmp)
	{
		return false;
	}

	break_jmp->opcode = OPCODE_JMP;
	break_jmp->operand1.type = OPERAND_TYPE_REFERENCE;
	break_jmp->operand1.ref = closure->break_to_instruction;
	add_instruction_to_closure(break_jmp, closure);
	return true;
}

bool generate_call_function(statement_call_function_t * call_function, closure_t * closure, type_space_t * type_space)
{
	return false;
}

bool parse_block(code_block_t * code_block, closure_t * closure, type_space_t *type_space)
{
	statement_t * current_statement = code_block->first_line;
	type_space_t * new_block_space = create_empty_type_space(type_space);
	if (NULL == new_block_space) {
		printf("Failed allocating new type space");
		return false;
	}

	if (!type_check_block(new_block_space, code_block, closure)) {
		return false;
	}

	while (current_statement != NULL) {
		switch (current_statement->statement_type) {
			case STATEMENT_TYPE_DECLARATION:
				if (!generate_declaration(current_statement, closure, new_block_space)) {
					return false;
				}
				break;

			case STATEMENT_TYPE_EXPRESSION:
				if (!generate_expression(&current_statement->expression, closure, new_block_space)) {
					return false;
				}
				break;

			case STATEMENT_TYPE_IFELSE:
				if (!generate_ifelse(&current_statement->ifelse, closure, new_block_space))
				{
					return false;
				}
				break;
			case STATEMENT_TYPE_LOOP:
				if (!generate_loop(&current_statement->loop, closure, new_block_space))
				{
					return false;
				}
				break;
			case STATEMENT_TYPE_BREAK:
				if (!generate_break(closure, new_block_space))
				{
					return false;
				}
				break;
			case STATEMENT_TYPE_CALL_FUNCTION:
				if (!generate_call_function(&current_statement->call_function, closure, new_block_space))
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
	"call",
	"push",
	"pop",
	"jz",
	"jnz",
	"nop",
	"neg",
	"seta",
	"setae",
	"setb",
	"setbe",
	"setc",
	"sete",
	"setne",
	"setg",
	"setge",
	"setl",
	"setle",
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

bool generate_assembly(closure_t * closure, int out_fd) {
	/* TODO: write to file, nigga */
	char * current_instruction_text = malloc(sizeof(*current_instruction_text) * 256);
	asm_node_t * current_instruction = NULL;
	
	if (NULL == current_instruction_text) {
		return false;
	}

	while (closure != NULL)
	{
		current_instruction = closure->instructions;


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
		closure = closure->next_closure;
	}
	return true;
}

bool initialize_type_generation(type_t * added_type) {
	if (NULL == added_type) {
		return false;
	}

	/* test for pointer first */
	if (added_type->deref_count > 0) {
		/* TODO: initialize with pointer function */
	}

	switch (added_type->type) {
		case DECLARATION_TYPE_BASE_PRIMITIVE:
			/* TODO: typedef with primitive */
			break;
		case DECLARATION_TYPE_BASE_CUSTOM_TYPE:
			return false; /* this should not happen because our type checker resolves typedefs */
		case DECLARATION_TYPE_BASE_STRUCT:
			/* TODO: struct definition or typedef with struct */
			break;
		case DECLARATION_TYPE_BASE_UNION:
			/* TODO: union definitions or typedef with union */
			break;
		case DECLARATION_TYPE_BASE_ENUM:
			/* TODO */
			break;
	}
	return false;
}

static type_space_t *generate_top_level_type_space() {
	bool success = false;
	type_t *current_primitive = NULL;
	type_space_t *top_level_space = create_empty_type_space(NULL);
	if (NULL == top_level_space) {
		goto cleanup;
	}

	/* TODO: add primitive variable allocation function */

	/* initialize primitives */
	current_primitive = add_primitive(top_level_space, "int", 4);
	if (NULL == current_primitive) {
		goto cleanup;
	}
	current_primitive->allocate_variable = &int_allocate_variable;

	if (NULL == add_primitive(top_level_space, "char", 1)) {
		goto cleanup;
	}

	if (NULL == add_primitive(top_level_space, "short", 2)) {
		goto cleanup;
	}

	success = true;

cleanup:
	if (!success && NULL != top_level_space) {
		free(top_level_space);
		top_level_space = NULL;
	}
	return top_level_space;
}

bool gen_asm_x86(function_node_t * function_list, int out_fd)
{
	function_node_t * current_function = function_list;
	function_parameter_t *current_parameter = NULL;
	type_space_t *top_level_type_space = NULL;
	type_t *current_parameter_type = NULL;
	closure_t *top_level_closure = NULL;

	top_level_type_space = generate_top_level_type_space();
	if (NULL == top_level_type_space) {
		return false;
	}

	top_level_closure = enter_new_closure(NULL, "global_closure");
	if (NULL == top_level_closure) {
		return false;
	}

	while (current_function != NULL)
	{
		type_space_t * type_space = create_empty_type_space(top_level_type_space);
		closure_t *function_closure = enter_new_closure(top_level_closure, current_function->function->identifier);

		/* add parameters to the function closure */
		current_parameter = current_function->function->parameter_list;
		while (current_parameter != NULL) {
			statement_declaration_t parameter_declaration = {
				.type = current_parameter->parameter_type,
				.identifier = current_parameter->parameter_identifier
			};
			current_parameter_type = get_declaration_type(type_space, &parameter_declaration);
			if (NULL == current_parameter) {
				/* unrecognized parameter type */
				return false;
			}
			if (!allocate_variable(
				function_closure,
				current_parameter->parameter_identifier,
				VALUE_TYPE_PARAMETER,
				current_parameter_type
			)) {
				return false;
			}
			current_parameter = current_parameter->next;
		}

		code_block_t * code_block = current_function->function->function_code;
		if (NULL == type_space) {
			printf("Failed allocating type space");
			return false;
		}

		if (!parse_block(code_block, function_closure, type_space)) {
			printf("Failed parsing to intermediate RISC\nGenerating assembly anyway.\n");
		}

		if (!generate_assembly(function_closure, out_fd))
		{
			return false;
		}
		current_function = current_function->next;
	}
	return true;
}