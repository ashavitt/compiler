#include <type_check.h>
#include <x86/closure.h>
#include <x86/gen_asm.h>

static bool is_lvalue(
	type_space_t *type_space,
	statement_expression_t *expression
) {
	return expression->expression_type == EXPRESSION_TYPE_IDENTIFIER; /* ||
		   expression->type == EXPRESSION_TYPE_OP && expression->exp_op.op == OP_DEREF ||
		   expression->type == EXPRESSION_TYPE_OP && expression->exp_op.op == OP_ARRAY_ACCESS */
}

bool type_check_expression(
	type_space_t *type_space,
	statement_expression_t *expression,
	closure_t *closure
) {
	type_t *exp1_type = NULL, *exp2_type = NULL, *exp3_type = NULL;
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

	/* type check the subexpressions */
	if ((NULL != expression->exp_op.exp1) &&
		!type_check_expression(type_space, expression->exp_op.exp1, closure)
	) {
		return false;
	}
	if (expression->exp_op.exp1 != NULL) {
		exp1_type = expression->exp_op.exp1->type;
	}
	if ((NULL != expression->exp_op.exp2) &&
		!type_check_expression(type_space, expression->exp_op.exp2, closure)
	) {
		return false;
	}
	if (expression->exp_op.exp2 != NULL) {
		exp2_type = expression->exp_op.exp2->type;
	}
	if ((NULL != expression->exp_op.exp3) &&
		!type_check_expression(type_space, expression->exp_op.exp3, closure)
	) {
		return false;
	}
	if (expression->exp_op.exp3 != NULL) {
		exp3_type = expression->exp_op.exp3->type;
	}

	switch (expression->exp_op.op) {
		case OP_MUL:
		case OP_PLUS:
		case OP_DIV:
		case OP_SUB:
		case OP_ADD:
			expression->type = exp1_type;
			return is_same_type(type_space, exp1_type, exp2_type) && /* we are yet to support conversions */
				   exp1_type->type == DECLARATION_TYPE_BASE_PRIMITIVE &&
				   exp2_type->type == DECLARATION_TYPE_BASE_PRIMITIVE;
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
				/* weird internal error */
				return false;
			}
			/* we don't support conversion yet */
			return is_same_type(type_space, exp1_type, exp2_type) &&
				   (exp1_type->type == DECLARATION_TYPE_BASE_PRIMITIVE ||
					exp1_type->deref_count > 0);
		case OP_ASSIGN:
			expression->type = exp1_type;
			/* we don't support conversion yet */
			return is_same_type(type_space, exp1_type, exp2_type) &&
				   is_lvalue(type_space, expression->exp_op.exp1);
		default:
			/* unhandled operation */
			return false;
	}

	return true;
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