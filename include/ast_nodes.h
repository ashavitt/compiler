#ifndef __AST_NODES_H__
#define __AST_NODES_H__

#include <stdbool.h>

/* Structs and functions regarding the operations in C without flow control */

typedef struct expression_op expression_op_t;
typedef struct statement_expression statement_expression_t;

typedef enum expression_type
{
	EXPRESSION_TYPE_OP,
	EXPRESSION_TYPE_CONST,
	EXPRESSION_TYPE_IDENTIFIER
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
	OP_NEQUAL,
	OP_BSHIFT_RIGHT,
	OP_BSHIFT_LEFT
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
	statement_expression_t * exp3
);

statement_expression_t * create_const_expression(
	long value
);

statement_expression_t * create_identifier_expression(
	const char * identifier
);

typedef enum declaration_type_base_e {
    DECLARATION_TYPE_BASE_PRIMITIVE,
    DECLARATION_TYPE_BASE_STRUCT,
    DECLARATION_TYPE_BASE_ENUM,
    DECLARATION_TYPE_BASE_CUSTOM_TYPE
} declaration_type_base_t;

typedef struct declaration_type_modifier_s {
    bool is_const;
    bool is_volatile;
    bool is_unsigned;
    bool is_register;
} declaration_type_modifier_t;

typedef enum declaration_type_base_type_primitive_e {
    DECLARATION_TYPE_BASE_TYPE_INT,
    DECLARATION_TYPE_BASE_TYPE_SHORT,
    DECLARATION_TYPE_BASE_TYPE_LONG,
    DECLARATION_TYPE_BASE_TYPE_LONG_LONG,
    DECLARATION_TYPE_BASE_TYPE_CHAR
} declaration_type_base_type_primitive_t;

typedef struct declaration_type_base_type_s {
    bool is_primitive;
    union {
        declaration_type_base_type_primitive_t primitive;
        char * type;
    };
} declaration_type_base_type_t;

/*!
 * TODO: add linkage type (static/extern)
 */
typedef struct declaration_type_s
{
    declaration_type_base_t type_base;
    declaration_type_base_type_t type_base_type;
    unsigned long deref_count;
    declaration_type_modifier_t modifier;
} declaration_type_t;

typedef struct statement_declaration
{
    declaration_type_t type;
	char * identifier;
} statement_declaration_t;

statement_declaration_t * create_declaration(
	declaration_type_base_type_primitive_t type,
	const char * identifier);

#endif
