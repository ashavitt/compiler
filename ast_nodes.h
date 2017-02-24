#ifndef __AST_NODES_H__
#define __AST_NODES_H__

typedef struct expression_op expression_op_t;
typedef struct statement_expression statement_expression_t;

typedef enum expression_type
{
	EXP_OP,
	EXP_CONST,
	EXP_IDENTIFIER
} expression_type_e;

typedef enum operator_type
{
	OP_ADD,
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
	OP_NEQUAL
} operator_type_e;

struct expression_op
{
	operator_type_e op;
	statement_expression_t * exp1;
	statement_expression_t * exp2;
	statement_expression_t * exp3;
};

struct statement_expression
{
	expression_type_e type;
	union
	{
		expression_op_t exp_op;
		long constant;
		char * identifier;
	};
};

statement_expression_t * create_op_expression(
	operator_type_e op,
	statement_expression_t * exp1,
	statement_expression_t * exp2,
	statement_expression_t * exp3);

statement_expression_t * create_const_expression(
	long value);

statement_expression_t * create_identifier_expression(
	const char * identifier);


typedef enum declaration_type
{
	DECL_CHAR,
	DECL_INT,
	DECL_LONG
} declaration_type_e;

typedef struct statement_declaration
{
	declaration_type_e type;
	char * identifier;
} statement_declaration_t;

statement_declaration_t * create_declaration(
	declaration_type_e type,
	const char * identifier);

#endif
