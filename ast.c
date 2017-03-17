#include <ast.h>
#include <ast_nodes.h>
#include <stdlib.h>
#include <stdio.h>

#define debug_shift_width (4)

const char * operator_type_str[] = {
	"addition", "substitution", "multiplication", "division", "modulu",
	"bitwise xor", "bitwise and", "bitwise or", "logical and", "logical or",
	"assignment", "unary plus", "unary minus", "equal", "not equal",
	"bitwise shift right", "bitwise shift left"
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
	statement_t * stmt = create_statement(STATEMENT_TYPE_EXPRESSION);
	if (NULL == stmt)
	{
		return NULL;
	}
	stmt->expression = *expr;
	return stmt;
}

statement_t * create_statement_declaration(statement_declaration_t * decl)
{
	statement_t * stmt = create_statement(STATEMENT_TYPE_DECLARATION);
	if (NULL == stmt)
	{
		return NULL;
	}
	stmt->declaration = *decl;
	return stmt;
}

statement_t * create_statement_ifelse(statement_ifelse_t * ifelse)
{
	statement_t * stmt = create_statement(STATEMENT_TYPE_IFELSE);
	if (NULL == stmt)
	{
		return NULL;
	}
	stmt->ifelse = *ifelse;
	return stmt;
}

statement_t * create_statement_loop(statement_loop_t * loop)
{
	statement_t * stmt = create_statement(STATEMENT_TYPE_LOOP);
	if (NULL == stmt)
	{
		return NULL;
	}
	stmt->loop = *loop;
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
		case EXPRESSION_TYPE_OP:
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
		case EXPRESSION_TYPE_CONST:
			printf("constant\n");
			printf("%*s%ld\n", (offset + 1) * debug_shift_width, "", expr->constant);
			break;
		case EXPRESSION_TYPE_IDENTIFIER:
			printf("identifier\n");
			printf("%*s%s\n", (offset + 1) * debug_shift_width, "", expr->identifier);
			break;
	}
}

char* get_primitive_string(declaration_type_base_type_primitive_t primitive) {
    switch (primitive) {
        case DECLARATION_TYPE_BASE_TYPE_INT:
            return "int";
        case DECLARATION_TYPE_BASE_TYPE_CHAR:
            return "char";
        case DECLARATION_TYPE_BASE_TYPE_LONG:
            return "long";
        case DECLARATION_TYPE_BASE_TYPE_LONG_LONG:
            return "long long";
        case DECLARATION_TYPE_BASE_TYPE_SHORT:
            return "short";
    }
    return "Unknown primitive, internal error";
}

void print_declaration(declaration_type_t * declaration) {
    unsigned long deref_count = declaration->deref_count;

    if (declaration->modifier.is_const) {
        printf("const ");
    }
    if (declaration->modifier.is_unsigned) {
        printf("unsigned ");
    }
    if (declaration->modifier.is_volatile) {
        printf("volatile ");
    }
    if (declaration->modifier.is_register) {
        printf("register ");
    }

    if (declaration->type_base == DECLARATION_TYPE_BASE_PRIMITIVE) {
        printf("%s ", get_primitive_string(declaration->type_base_type.primitive));
    }
    else if (declaration->type_base == DECLARATION_TYPE_BASE_STRUCT ||
             declaration->type_base == DECLARATION_TYPE_BASE_ENUM ||
             declaration->type_base == DECLARATION_TYPE_BASE_CUSTOM_TYPE
    ) {
        printf("%s ", declaration->type_base_type.type);
    }
    else {
        printf("Unknown type, internal error");
        return;
    }

    while (deref_count > 0) {
        printf("*");
        --deref_count;
    }
}

void debug_declaration(statement_t * declaration, int offset)
{
	statement_declaration_t decl = declaration->declaration;
	printf("%*sDeclaration type: ", offset * debug_shift_width, "");
    print_declaration(&decl.type);
	printf("\n%*sDeclaration identifier: %s\n", offset * debug_shift_width, "", decl.identifier);
}

void debug_ifelse(statement_ifelse_t * ifelse, int offset)
{
	printf("%*sIF\n", offset * debug_shift_width, "");
	debug_expression(ifelse->if_expr, offset + 1);
	printf("%*sTHEN\n", offset * debug_shift_width, "");
	debug_code_block(ifelse->if_block, offset + 1);
	if (ifelse->else_block != NULL)
	{
		printf("%*sELSE\n", offset * debug_shift_width, "");
		debug_code_block(ifelse->else_block, offset + 1);
	}
}

void debug_loop(statement_loop_t * loop, int offset)
{
	if (loop->init_expression != NULL)
	{
		printf("%*sINIT\n", offset * debug_shift_width, "");
		debug_expression(loop->init_expression, offset + 1);
	}
	if (loop->condition_expression != NULL)
	{
		printf("%*sCONDITION\n", offset * debug_shift_width, "");
		debug_expression(loop->condition_expression, offset + 1);
	}
	if (loop->iteration_expression != NULL)
	{
		printf("%*sITERATION\n", offset * debug_shift_width, "");
		debug_expression(loop->iteration_expression, offset + 1);
	}
	printf("%*sLOOP BODY\n", offset * debug_shift_width, "");
	debug_code_block(loop->loop_body, offset + 1);
}

void debug_code_block(
	code_block_t * code_block, int offset)
{
	statement_t * statement = NULL;
	statement = code_block->first_line;
	while (NULL != statement)
	{
		printf("%*sStatement type: ", offset * debug_shift_width, "");
		switch(statement->statement_type)
		{
			case STATEMENT_TYPE_DECLARATION:
				printf("declaration\n");
				debug_declaration(statement, offset + 1);
				break;
			case STATEMENT_TYPE_EXPRESSION:
				printf("expression\n");
				debug_expression(&(statement->expression), offset + 1);
				break;
			case STATEMENT_TYPE_IFELSE:
				printf("ifelse block\n");
				debug_ifelse(&(statement->ifelse), offset + 1);
				break;
			case STATEMENT_TYPE_LOOP:
				printf("loop block\n");
				debug_loop(&(statement->loop), offset + 1);
				break;
		}
		statement = statement->next;
	}
}

void debug_ast(
	code_file_t * code_file)
{
	debug_code_block(code_file->first_block, 0);
}
