#include "ast.h"
#include "ast_nodes.h"
#include <stdlib.h>
#include <stdio.h>

#define debug_shift_width (4)

const char * operator_type_str[] = {
	"addition", "substitution", "multiplication", "division", "modulu",
	"bitwise xor", "bitwise and", "bitwise or", "logical and", "logical or",
	"assignment", "unary plus", "unary minus"
};

statement_t * create_statement(statement_type_e type)
{
	statement_t * stmt = (statement_t *) malloc (sizeof(statement_t));
	if (NULL == stmt)
	{
		return NULL;
	}
	stmt->statement_type = type;
	stmt->next = NULL;
	return stmt;
}

statement_t * create_statement_expression(statement_expression_t * expr)
{
	statement_t * stmt = create_statement(EXPRESSION);
	if (NULL == stmt)
	{
		return NULL;
	}
	stmt->expression = *expr;
	return stmt;
}

statement_t * create_statement_declaration(statement_declaration_t * decl)
{
	statement_t * stmt = create_statement(DECLARATION);
	if (NULL == stmt)
	{
		return NULL;
	}
	stmt->declaration = *decl;
	return stmt;
}

statement_t * create_statement_ifelse(statement_ifelse_t * ifelse)
{
	statement_t * stmt = create_statement(IFELSE);
	if (NULL == stmt)
	{
		return NULL;
	}
	stmt->ifelse = *ifelse;
	return stmt;
}

void add_statement(
	code_block_t * code_block,
	statement_t * statement)
{
	statement_t * next_stmt = code_block->first_line;
	while (NULL != next_stmt->next)
	{
		next_stmt = next_stmt->next;
	}
	next_stmt->next = statement;
}

void debug_expression(statement_expression_t * expr, int offset)
{
	printf("%*sExpression type: ", offset * debug_shift_width, "");
	switch (expr->type)
	{
		case EXP_OP:
			printf("operator\n");
			printf("%*soperator: %s\n", (offset + 1) * debug_shift_width, "", operator_type_str[expr->exp_op.op]);
			if (NULL != expr->exp_op.exp1)
			{
				debug_expression(expr->exp_op.exp1, offset + 2);
			}
			if (NULL != expr->exp_op.exp2)
			{
				debug_expression(expr->exp_op.exp2, offset + 2);
			}
			if (NULL != expr->exp_op.exp3)
			{
				debug_expression(expr->exp_op.exp3, offset + 2);
			}
			break;
		case EXP_CONST:
			printf("constant\n");
			printf("%*s%ld\n", (offset + 1) * debug_shift_width, "", expr->constant);
			break;
		case EXP_IDENTIFIER:
			printf("identifier\n");
			printf("%*s%s\n", (offset + 1) * debug_shift_width, "", expr->identifier);
			break;
	}
}

void debug_declaration(statement_t * declaration, int offset)
{
	statement_declaration_t decl = declaration->declaration;
	printf("%*sDeclaration type: ", offset * debug_shift_width, "");
	switch (decl.type)
	{
		case DECL_CHAR:
			printf("char");
			break;
		case DECL_INT:
			printf("int");
			break;
		case DECL_LONG:
			printf("long");
			break;
	}
	printf("\n%*sDeclaration identifier: %s\n", offset * debug_shift_width, "", decl.identifier);
}

void debug_ifelse(statement_ifelse_t * ifelse, int offset)
{
	printf("%*sIF\n", offset * debug_shift_width, "");
	debug_expression(ifelse->if_expr, offset + 1);
	printf("%*sTHEN\n", offset * debug_shift_width, "");
	printf("%*sELSE\n", offset * debug_shift_width, "");
}

void debug_ast(
	code_file_t * code_file)
{
	statement_t * statement = NULL;
	statement = code_file->first_block->first_line;
	while (NULL != statement)
	{
		printf("Statement type: ");
		switch(statement->statement_type)
		{
			case DECLARATION:
				printf("declaration\n");
				debug_declaration(statement, 1);
				break;
			case EXPRESSION:
				printf("expression\n");
				debug_expression(&(statement->expression), 1);
				break;
			case IFELSE:
				printf("ifelse block\n");
				debug_ifelse(&(statement->ifelse), 1);
				break;
		}
		statement = statement->next;
	}
}
