#include <ast.h>
#include <ast_nodes.h>
#include <stdlib.h>
#include <stdio.h>

#define debug_shift_width (4)

const char * operator_type_str[] = {
	"addition", "substitution", "multiplication", "division", "modulu",
	"bitwise xor", "bitwise and", "bitwise or", "logical and", "logical or",
	"assignment", "unary plus", "unary minus", "equal", "not equal",
	"bitwise shift right", "bitwise shift left", "less", "greater",
	"less equal", "greater equal", "ternary condition"
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

statement_t * create_statement_declaration(
	statement_declaration_t * decl,
	statement_expression_t * initial_value)
{
	statement_t * stmt = create_statement(STATEMENT_TYPE_DECLARATION);
	if (NULL == stmt)
	{
		return NULL;
	}
	stmt->declaration = *decl;
	
	/* syntactic sugar, assign a value at declaration */
	if (initial_value != NULL)
	{
		statement_expression_t * declaration_identifier = create_identifier_expression(decl->identifier);
		statement_expression_t * set_initial_value = create_op_expression(OP_ASSIGN, declaration_identifier, initial_value, NULL);
		statement_t * value_stmt = create_statement_expression(set_initial_value);
		stmt->next = value_stmt;
	}
	return stmt;
}

statement_t * create_statement_type_declaration(statement_type_declaration_t * decl)
{
	statement_t * stmt = create_statement(STATEMENT_TYPE_TYPE_DECLARATION);
	if (NULL == stmt)
	{
		return NULL;
	}
	stmt->type_declaration = *decl;
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

statement_t * create_statement_break()
{
	statement_t * stmt = create_statement(STATEMENT_TYPE_BREAK);
	if (NULL == stmt)
	{
		return NULL;
	}
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

void print_declaration(declaration_type_t * declaration, int offsets);

void print_fields(field_t *fields, int offset) {
    while (fields != NULL) {
        printf("%*s", offset * debug_shift_width, "");
        print_declaration(&fields->declaration->type, offset);
        printf(" %s;\n", fields->declaration->identifier);
        fields = fields->next;
    }
}

void print_enum_fields(field_t *fields, int offset) {
    while (fields != NULL) {
        if (fields->next != NULL) {
            printf("%*s%s = %lu,\n", offset * debug_shift_width, "", fields->declaration->identifier, fields->declaration->type.enum_value);
        }
        else{
            printf("%*s%s = %lu\n", offset * debug_shift_width, "", fields->declaration->identifier, fields->declaration->type.enum_value);
        }
        fields = fields->next;
    }
}

void print_declaration(declaration_type_t * declaration, int offset) {
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
        printf("%s ", declaration->type_base_type.identifier);
    }
    else if (declaration->type_base == DECLARATION_TYPE_BASE_STRUCT) {
        printf("struct { \n");
        print_fields(declaration->type_base_type.fields, offset+1);
        printf("%*s};", offset * debug_shift_width, "");
    } else if (declaration->type_base == DECLARATION_TYPE_BASE_UNION) {
        printf("union { \n");
        print_fields(declaration->type_base_type.fields, offset+1);
        printf("%*s};", offset * debug_shift_width, "");
    } else if (declaration->type_base == DECLARATION_TYPE_BASE_ENUM) {
        printf("enum { \n");
        print_enum_fields(declaration->type_base_type.fields, offset+1);
        printf("%*s};", offset * debug_shift_width, "");
    } else if (declaration->type_base == DECLARATION_TYPE_BASE_CUSTOM_TYPE) {
        printf("(");
        print_declaration(&declaration->type_base_type.typedef_type->type, offset);
        printf(")");
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

void debug_type_declaration(statement_t * declaration, int offset)
{
    statement_type_declaration_t decl = declaration->type_declaration;
    printf("%*sType Declaration type: ", offset * debug_shift_width, "");
    print_declaration(&decl.type, offset);
    printf("\n%*sType Declaration identifier: %s\n", offset * debug_shift_width, "", decl.type_name);
}

void debug_declaration(statement_t * declaration, int offset)
{
	statement_declaration_t decl = declaration->declaration;
	printf("%*sDeclaration type: ", offset * debug_shift_width, "");
    print_declaration(&decl.type, offset);
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
			case STATEMENT_TYPE_TYPE_DECLARATION:
				printf("type declaration\n");
				debug_type_declaration(statement, offset+1);
				break;
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
			case STATEMENT_TYPE_BREAK:
				printf("break\n");
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
