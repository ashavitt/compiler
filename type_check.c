#include <types/type_check.h>
#include <x86/closure.h>
#include <x86/gen_asm.h>
#include <memory.h>
#include <x86/types/int.h>

bool is_lvalue(
	type_space_t *type_space,
	statement_expression_t *expression
) {
	return expression->expression_type == EXPRESSION_TYPE_IDENTIFIER ||
		   expression->expression_type == EXPRESSION_TYPE_OP && expression->exp_op.op == OP_DREF; /* ||
		   expression->type == EXPRESSION_TYPE_OP && expression->exp_op.op == OP_ARRAY_ACCESS */
}

static type_t * deref_of(type_t * pointer_type) {
	type_t * deref_type = malloc(sizeof(*deref_type));
	if (NULL == deref_type) {
		return NULL;
	}

	memcpy(deref_type, pointer_type, sizeof(*deref_type));
	deref_type->deref_count -= 1;
	return deref_type;
}

static type_t * pointer_to(type_t * lvalue_type) {
	type_t * pointer_type = malloc(sizeof(*pointer_type));
	if (NULL == pointer_type) {
		return NULL;
	}

	memcpy(pointer_type, lvalue_type, sizeof(*pointer_type));
	pointer_type->deref_count += 1;
	return pointer_type;
}

static bool type_check_add(
	type_space_t *type_space,
	statement_expression_t * expression,
	closure_t *closure
) {
	type_t *exp1_type = NULL, *exp2_type = NULL;

	/* type check the subexpressions */
	if ((NULL == expression->exp_op.exp1) || (NULL == expression->exp_op.exp2)) {
		return false;
	}

	if (
		!type_check_expression(type_space, expression->exp_op.exp1, closure) ||
		!type_check_expression(type_space, expression->exp_op.exp2, closure)
	) {
		return false;
	}

	exp1_type = expression->exp_op.exp1->type;
	exp2_type = expression->exp_op.exp2->type;

	if (
		!is_same_type(type_space, exp1_type, exp2_type) && /* we are yet to support conversions */
		(exp1_type->type != DECLARATION_TYPE_BASE_PRIMITIVE) &&
		(exp2_type->type != DECLARATION_TYPE_BASE_PRIMITIVE)
	) {
		return false;
	}

	/* set the type of the expression */
	expression->type = expression->exp_op.exp1->type;
	expression->generation_function = generate_int_addition;
	return true;
}

static bool type_check_equal(
	type_space_t *type_space,
	statement_expression_t * expression,
	closure_t *closure
) {
	type_t *exp1_type = NULL, *exp2_type = NULL;

	/* type check the subexpressions */
	if ((NULL == expression->exp_op.exp1) || (NULL == expression->exp_op.exp2)) {
		return false;
	}

	if (
		!type_check_expression(type_space, expression->exp_op.exp1, closure) ||
		!type_check_expression(type_space, expression->exp_op.exp2, closure)
	) {
		return false;
	}

	exp1_type = expression->exp_op.exp1->type;
	exp2_type = expression->exp_op.exp2->type;

	if (
		!is_same_type(type_space, exp1_type, exp2_type) && /* we are yet to support conversions */
		(exp1_type->type != DECLARATION_TYPE_BASE_PRIMITIVE) &&
		(exp2_type->type != DECLARATION_TYPE_BASE_PRIMITIVE)
	) {
		return false;
	}

	/* set the type of the expression */
	expression->type = expression->exp_op.exp1->type;
	expression->generation_function = generate_int_equal;
	return true;
}

static bool type_check_assigment(
	type_space_t *type_space,
	statement_expression_t * expression,
	closure_t *closure
) {
	type_t *exp1_type = NULL, *exp2_type = NULL;

	/* type check the subexpressions */
	if ((NULL == expression->exp_op.exp1) || (NULL == expression->exp_op.exp2)) {
		return false;
	}

	if (
		!type_check_expression(type_space, expression->exp_op.exp1, closure) ||
		!type_check_expression(type_space, expression->exp_op.exp2, closure)
	) {
		return false;
	}

	exp1_type = expression->exp_op.exp1->type;
	exp2_type = expression->exp_op.exp2->type;

	if (
		!is_same_type(type_space, exp1_type, exp2_type) && /* we are yet to support conversions */
		!is_lvalue(type_space, expression->exp_op.exp1)
	) {
		return false;
	}
	/* set the type of the expression */
	expression->type = expression->exp_op.exp1->type;
	expression->generation_function = generate_int_assignment;
	return true;
}

static bool type_check_dref(
	type_space_t *type_space,
	statement_expression_t * expression,
	closure_t *closure
) {
	/* type check the subexpressions */
/*	if ((NULL != expression->exp_op.exp1) &&
		!type_check_expression(type_space, expression->exp_op.exp1, closure)
		) {
		return false;
	}
	if (expression->exp_op.exp1 == NULL) {
		return false;
	}

	if (!type_check_expression(type_space, expression->exp_op.exp1, closure)) {
		return false;
	}

	if (expression->exp_op.exp1->type->deref_count <= 0) {
		return false;
	}

	expression->generation_function =*/
	return false;
}

static bool type_check_ref(
	type_space_t *type_space,
	statement_expression_t * expression,
	closure_t *closure
) {
	return false;
}

/*
 * 	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_BXOR,
	OP_BAND,
	OP_BOR,
	OP_AND,
	OP_OR,
	OP_ASSIGN,
	OP_PLUS,
	OP_NEG,
	OP_EQUAL,
	OP_NEQUAL,
	OP_BSHIFT_RIGHT,
	OP_BSHIFT_LEFT,
	OP_LESS,
	OP_GREATER,
	OP_LESS_EQUAL,
	OP_GREATER_EQUAL,
	OP_TERNARY,
	OP_DREF,
	OP_REF
 */

static bool(*operation_type_checks[])(
	type_space_t *type_space,
	statement_expression_t * expression,
	closure_t *closure
) = {
	type_check_add,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	type_check_assigment,
	NULL,
	NULL,
	type_check_equal,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	type_check_dref,
	type_check_ref
};

bool type_check_expression(
	type_space_t *type_space,
	statement_expression_t *expression,
	closure_t *closure
) {
	variable_t *variable = NULL;

	if (EXPRESSION_TYPE_CONST == expression->expression_type) {
		expression->type = lookup_type(type_space, "int");
		return true;
	} else if (EXPRESSION_TYPE_IDENTIFIER == expression->expression_type) {
		variable = get_variable(closure, expression->identifier);
		if (NULL == variable) {
			/* variable does not exist */
			return false;
		}
		expression->type = variable->type;
		return true;
	}

	/* assuming EXPRESSION_TYPE_OP */
	if (expression->expression_type != EXPRESSION_TYPE_OP) {
		/* unhandled expression type */
		return false;
	}

	/*switch (expression->exp_op.op) {
		case OP_MUL:
		case OP_PLUS:
		case OP_DIV:
		case OP_SUB:
		case OP_ADD:
			break;
		case OP_EQUAL:
		case OP_NEQUAL:
		case OP_OR:
		case OP_AND:
		case OP_GREATER:
		case OP_GREATER_EQUAL:
		case OP_LESS:
		case OP_LESS_EQUAL:
			expression->type = lookup_type(type_space, "int");
			if (NULL == expression->type) {
				*//* weird internal error *//*
				return false;
			}
			*//* we don't support conversion yet *//*
			return is_same_type(type_space, exp1_type, exp2_type) &&
				   (exp1_type->type == DECLARATION_TYPE_BASE_PRIMITIVE ||
					exp1_type->deref_count > 0);
		case OP_ASSIGN:
			expression->type = exp1_type;
			*//* we don't support conversion yet *//*
			return is_same_type(type_space, exp1_type, exp2_type) &&
				   is_lvalue(type_space, expression->exp_op.exp1);
		case OP_DREF:
			expression->type = deref_of(exp1_type);
			return (exp1_type->deref_count > 0);
		case OP_REF:
			expression->type = pointer_to(exp1_type);
			return is_lvalue(type_space, expression->exp_op.exp1);
		default:
			*//* unhandled operation *//*
			return false;
	}*/

	if (
		(expression->exp_op.op >= (sizeof(operation_type_checks)/sizeof(operation_type_checks[0]))) ||
		(NULL == operation_type_checks[expression->exp_op.op])
	) {
		/* not implemented */
		return false;
	}

	return operation_type_checks[expression->exp_op.op](type_space, expression, closure);
}

static bool type_check_declaration(
	type_space_t *type_space,
	statement_declaration_t *declaration,
	closure_t *closure
) {
	type_t *declaration_type = NULL;
	variable_t *added_variable = NULL;

	declaration_type = get_declaration_type(type_space, declaration);
	if (NULL == declaration_type) {
		return false;
	}
	/* TODO: call type's allocation function */
	/* we add variables as we type-check*/
	added_variable = allocate_variable(
		closure,
		declaration->identifier,
		VALUE_TYPE_VARIABLE,
		declaration_type
	);
	if (NULL == added_variable) {
		return false;
	}
	return true;
}

static bool type_check_loop(
	type_space_t *type_space,
	statement_loop_t *loop,
	closure_t *closure
) {
	type_t *condition_type = NULL;

	if ((NULL != loop->init_expression) && !type_check_expression(type_space, loop->init_expression, closure)) {
		return false;
	}
	if (!type_check_expression(type_space, loop->condition_expression, closure)) {
		return false;
	}
	condition_type = loop->condition_expression->type;
	if (
		(NULL != loop->iteration_expression) &&
		!type_check_expression(type_space, loop->iteration_expression, closure)
	) {
		return false;
	}
	/*if (!type_check_block(type_space, loop->loop_body, closure)) {
		return false;
	}*/
	/* TODO: shouldn't we have bool? */
	return is_same_type(type_space, condition_type, lookup_type(type_space, "int"));
}

static bool type_check_if(
	type_space_t *type_space,
	statement_ifelse_t *if_statement,
	closure_t *closure
) {
	type_t *if_type = NULL;
	if (
		!type_check_expression(type_space, if_statement->if_expr, closure) /* ||
		!type_check_block(type_space, if_statement->if_block, closure) ||
		!type_check_block(type_space, if_statement->else_block, closure) */
	) {
		return false;
	}
	if_type = if_statement->if_expr->type;
	/* TODO: shouldn't we have bool? */
	return is_same_type(type_space, if_type, lookup_type(type_space, "int"));
}

static bool type_check_statement(
	type_space_t *type_space,
	statement_t *statement,
	closure_t *closure
) {
	type_t * added_type = NULL;
	switch (statement->statement_type) {
		case STATEMENT_TYPE_DECLARATION:
			return type_check_declaration(type_space, &statement->declaration, closure);
		case STATEMENT_TYPE_TYPE_DECLARATION:
			added_type = add_type(type_space, &statement->type_declaration);
			if (NULL != added_type) {
				return false;
			}
			if (!initialize_type_generation(added_type)) {
				return false;
			}
			break;
		case STATEMENT_TYPE_LOOP:
			return type_check_loop(type_space, &statement->loop, closure);
		case STATEMENT_TYPE_IFELSE:
			return type_check_if(type_space, &statement->ifelse, closure);
		case STATEMENT_TYPE_EXPRESSION:
			return type_check_expression(type_space, &statement->expression, closure);
		case STATEMENT_TYPE_BREAK:
			break;
		default:
			/* unhandled statement type */
			return false;
	}

	return true;
}

bool type_check_block(type_space_t *type_space, code_block_t *code_block, closure_t *closure) {
	statement_t *current_statement = code_block->first_line;
	while (current_statement != NULL) {
		if (!type_check_statement(type_space, current_statement, closure)) {
			return false;
		}
		current_statement = current_statement->next;
	}
	return true;
}
