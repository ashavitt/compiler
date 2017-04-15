#include <x86/types/int.h>
#include <x86/gen_asm.h>
#include <x86/closure.h>
#include <string.h>

/* TODO: make difference between lvalue/rvalue and make lvalue promotion to rvalue,
 *       see how all of this goes together with pointers and stuff */

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

static bool load_int_from_stack(variable_t * variable, closure_t * closure, register_e target_register) {
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
	node->operand2.type = OPERAND_TYPE_STACK_DWORD;
	node->operand2.stack_offset = variable->position.stack_offset;

	add_instruction_to_closure(node, closure);
	return true;
}

static bool load_int_const_to_register(long constant, closure_t * closure, register_e target_register) {
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

static bool load_int_expression_to_register(
	statement_expression_t * expression,
	closure_t * closure,
	register_e target_register,
	type_space_t *type_space
) {
	variable_t * expression_result = NULL;

	switch (expression->expression_type) {
		case EXPRESSION_TYPE_CONST:
			return load_int_const_to_register(expression->constant, closure, target_register);
		case EXPRESSION_TYPE_IDENTIFIER:
			/* TODO: error handling */
			return load_int_from_stack(get_variable(closure, expression->identifier), closure, target_register);
			/* if we got here, our value is on the stack */
		default:
			expression_result = lookup_expression_result(expression, closure, type_space);
			if (NULL == expression_result) {
				return false;
			}
			return load_int_from_stack(expression_result, closure, target_register);
	}
}

bool generate_int_assignment(
	statement_expression_t * operation,
	closure_t * closure,
	type_space_t *type_space
) {
	expression_op_t * assignment = &operation->exp_op;

	/* we are yet to handle non-variable lvalues */
	if (assignment->exp1->expression_type != EXPRESSION_TYPE_IDENTIFIER) {
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
	if (assignment->exp1->expression_type == EXPRESSION_TYPE_IDENTIFIER) {
		if(!generate_expression(assignment->exp2, closure, type_space)) {
			return false;
		}

		if (!load_int_expression_to_register(assignment->exp2, closure, REGISTER_EAX, type_space)) {
			return false;
		}

		node->opcode = OPCODE_MOV;
		node->operand1.type = OPERAND_TYPE_STACK_DWORD;
		node->operand1.stack_offset = assigned_variable->position.stack_offset;
		node->operand2.type = OPERAND_TYPE_REG;
		node->operand2.reg = REGISTER_EAX;

		add_instruction_to_closure(node, closure);
		assigned_variable->evaluated_expression = operation;
	} else {
		return false;
	}

	return true;
}

static bool generate_int_arithmetic(
	statement_expression_t * expression,
	closure_t * closure,
	opcode_e opcode,
	type_space_t *type_space
) {
	bool success = false;
	variable_t * result = NULL;
	expression_op_t * arithmetic_expression = &expression->exp_op;
	asm_node_t * node_op = NULL;

	node_op =  malloc(sizeof(*node_op));
	if (NULL == node_op) {
		success = false;
		goto cleanup;
	}

	if(!generate_expression(arithmetic_expression->exp1, closure, type_space)) {
		return false;
	}

	if(!generate_expression(arithmetic_expression->exp2, closure, type_space)) {
		return false;
	}

	if (!load_int_expression_to_register(arithmetic_expression->exp1, closure, REGISTER_EAX, type_space)) {
		success = false;
		goto cleanup;
	}

	if (!load_int_expression_to_register(arithmetic_expression->exp2, closure, REGISTER_EBX, type_space)) {
		success = false;
		goto cleanup;
	}

	node_op->operand1.type = OPERAND_TYPE_REG;
	node_op->operand1.reg = REGISTER_EAX;
	node_op->operand2.type = OPERAND_TYPE_REG;
	node_op->operand2.reg = REGISTER_EBX;
	node_op->opcode = opcode;

	result = allocate_variable(closure, NULL, VALUE_TYPE_EXPRESSION_RESULT, arithmetic_expression->exp1->type);
	if (NULL == result) {
		success = false;
		goto cleanup;
	}

	add_instruction_to_closure(node_op, closure);
	if (!store_register_to_variable(result, REGISTER_EAX, closure)) {
		success = false;
		goto cleanup;
	}

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

static bool generate_int_unary_operator(
	statement_expression_t * expression,
	closure_t * closure,
	opcode_e opcode,
	type_space_t *type_space
) {
	bool success = false;
	asm_node_t * opcode_asm = malloc(sizeof(*opcode_asm));
	type_t *expression_type = NULL;

	if (NULL == opcode_asm) {
		goto cleanup;
	}

	if(!generate_expression(expression->exp_op.exp1, closure, type_space)) {
		goto cleanup;
	}

	if (!load_int_expression_to_register(expression->exp_op.exp1, closure, REGISTER_EAX, type_space))
	{
		goto cleanup;
	}

	expression_type = expression->exp_op.exp1->type;
	variable_t * result =  allocate_variable(closure, NULL, VALUE_TYPE_EXPRESSION_RESULT, expression_type);

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

bool generate_int_multiplication(
	statement_expression_t * expression,
	closure_t * closure,
	type_space_t * type_space
) {
	asm_node_t * mul_asm = NULL;
	variable_t * result = NULL;

	/* TODO: fix when there are conversions */
	result = allocate_variable(closure, NULL, VALUE_TYPE_EXPRESSION_RESULT, expression->exp_op.exp1->type);
	if (NULL == result) {
		return false;
	}

	if (!generate_expression(expression->exp_op.exp1, closure, type_space)) {
		return false;
	}

	if (!generate_expression(expression->exp_op.exp2, closure, type_space)) {
		return false;
	}

	if (!load_int_expression_to_register(expression->exp_op.exp1, closure, REGISTER_EBX, type_space)) {
		return false;
	}

	if (!load_int_expression_to_register(expression->exp_op.exp2, closure, REGISTER_EAX, type_space)) {
		return false;
	}

	mul_asm =  malloc(sizeof(*mul_asm));
	if (NULL == mul_asm) {
		return false;
	}

	mul_asm->opcode = OPCODE_MUL;
	mul_asm->operand1.type = OPERAND_TYPE_REG;
	mul_asm->operand1.reg = REGISTER_EBX;
	mul_asm->operand2.type = OPERAND_TYPE_NONE;
	add_instruction_to_closure(mul_asm, closure);
	mul_asm = NULL;
	if (!store_register_to_variable(result, REGISTER_EAX, closure)) {
		return false;
	}
	result->evaluated_expression = expression;
	return true;
}

static bool generate_int_comparison(
	statement_expression_t * expression,
	closure_t * closure,
	operator_type_e operator_type,
	type_space_t *type_space
) {
	asm_node_t * set_result_op = malloc(sizeof(*set_result_op));
	bool success = false;
	bool signed_operators = false;
	variable_t * result = NULL;
	expression_op_t * comparison_expression = &expression->exp_op;
	asm_node_t * substraction_op = NULL;
	opcode_e set_result_opcode;

	/* TODO set the signed_operators variable accordingly */

	substraction_op =  malloc(sizeof(*substraction_op));
	if (NULL == substraction_op) {
		success = false;
		goto cleanup;
	}

	/* compute the operands first */
	if (!generate_expression(comparison_expression->exp1, closure, type_space)) {
		success = false;
		return false;
	}

	if (!generate_expression(comparison_expression->exp2, closure, type_space)) {
		success = false;
		return false;
	}

	if (!load_int_expression_to_register(comparison_expression->exp1, closure, REGISTER_EAX, type_space)) {
		success = false;
		goto cleanup;
	}

	if (!load_int_expression_to_register(comparison_expression->exp2, closure, REGISTER_EBX, type_space)) {
		success = false;
		goto cleanup;
	}

	/* substract the operands for comparison */

	substraction_op->operand1.type = OPERAND_TYPE_REG;
	substraction_op->operand1.reg = REGISTER_EAX;
	substraction_op->operand2.type = OPERAND_TYPE_REG;
	substraction_op->operand2.reg = REGISTER_EBX;
	substraction_op->opcode = OPCODE_SUB;

	add_instruction_to_closure(substraction_op, closure);

	/* calculate the result */

	switch (comparison_expression->op)
	{
		case OP_EQUAL:
			set_result_opcode = OPCODE_SETE;
			break;
		case OP_NEQUAL:
			set_result_opcode = OPCODE_SETNE;
			break;
		case OP_LESS:
			if (signed_operators) {
				set_result_opcode = OPCODE_SETL;
			} else {
				set_result_opcode = OPCODE_SETB;
			}
			break;
		case OP_LESS_EQUAL:
			if (signed_operators) {
				set_result_opcode = OPCODE_SETLE;
			} else {
				set_result_opcode = OPCODE_SETBE;
			}
			break;
		case OP_GREATER:
			if (signed_operators) {
				set_result_opcode = OPCODE_SETG;
			} else {
				set_result_opcode = OPCODE_SETA;
			}
			break;
		case OP_GREATER_EQUAL:
			if (signed_operators) {
				set_result_opcode = OPCODE_SETGE;
			} else {
				set_result_opcode = OPCODE_SETAE;
			}
			break;
		default:
			/* unhandled */
			goto cleanup;
	}

	set_result_op->operand1.type = OPERAND_TYPE_REG;
	set_result_op->operand1.reg = REGISTER_ECX;
	set_result_op->opcode = set_result_opcode;

	add_instruction_to_closure(set_result_op, closure);

	type_t * int_type = lookup_type(type_space, "int");
	if (NULL == int_type) {
		/* internal error */
		goto cleanup;
	}
	result = allocate_variable(closure, NULL, VALUE_TYPE_EXPRESSION_RESULT, int_type);
	if (NULL == result) {
		success = false;
		goto cleanup;
	}

	if (!store_register_to_variable(result, REGISTER_ECX, closure)) {
		success = false;
		goto cleanup;
	}

	result->evaluated_expression = expression;

	success = true;

cleanup:
	if (!success) {
		if (substraction_op != NULL) {
			free(substraction_op);
		}
	}
	return success;
}

//static bool generate_int_operation(statement_expression_t * expression, closure_t * closure, type_space_t *type_space) {
//	switch (expression->exp_op.op) {
//		case OP_ASSIGN:
//			return generate_int_assignment(expression, closure, type_space);
//		case OP_ADD:
//			return generate_int_arithmetic(expression, closure, OPCODE_ADD, type_space);
//		case OP_BAND:
//			return generate_int_arithmetic(expression, closure, OPCODE_AND, type_space);
//		case OP_BOR:
//			return generate_int_arithmetic(expression, closure, OPCODE_OR, type_space);
//		case OP_SUB:
//			return generate_int_arithmetic(expression, closure, OPCODE_SUB, type_space);
//		case OP_NEG:
//			return generate_int_unary_operator(expression, closure, OPCODE_NEG, type_space);
//		case OP_MUL:
//			return generate_int_multiplication(expression, closure, type_space);
//		case OP_PLUS:
//			/* basically just transfer expression result to the current one */
//			if (!generate_expression(expression->exp_op.exp1, closure, type_space)) {
//				return false;
//			}
//			variable_t * result = lookup_expression_result(expression->exp_op.exp1, closure, type_space);
//			if (result == NULL) {
//				return false;
//			}
//			result->evaluated_expression = expression;
//			return true;
//		case OP_EQUAL:
//		case OP_NEQUAL:
//		case OP_LESS:
//		case OP_LESS_EQUAL:
//		case OP_GREATER:
//		case OP_GREATER_EQUAL:
//			return generate_int_comparison(expression, closure, expression->exp_op.op, type_space);
//		default:
//			return false;
//	}
//}

bool generate_int_addition(
	statement_expression_t * operation,
	closure_t * closure,
	type_space_t *type_space
) {
	return generate_int_arithmetic(operation, closure, OPCODE_ADD, type_space);
};

bool generate_int_substraction(
	statement_expression_t * operation,
	closure_t * closure,
	type_space_t *type_space
) {
	return generate_int_arithmetic(operation, closure, OPCODE_SUB, type_space);
};
