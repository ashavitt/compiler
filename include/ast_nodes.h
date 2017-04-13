#ifndef __AST_NODES_H__
#define __AST_NODES_H__

#include <stdbool.h>

/* Structs and functions regarding the operations in C without flow control */

typedef struct expression_op expression_op_t;
typedef struct statement_expression statement_expression_t;
typedef struct statement_declaration statement_declaration_t;
typedef struct statement_type_declaration statement_type_declaration_t;

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
	OP_BSHIFT_LEFT,
	OP_LESS,
	OP_GREATER,
	OP_LESS_EQUAL,
	OP_GREATER_EQUAL,
	OP_TERNARY
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
    DECLARATION_TYPE_BASE_UNION,
    DECLARATION_TYPE_BASE_CUSTOM_TYPE
} declaration_type_base_t;

typedef struct declaration_type_modifier_s {
    bool is_const;
    bool is_volatile;
    bool is_unsigned;
    bool is_register;
} declaration_type_modifier_t;

typedef struct declaration_type_s declaration_type_t;

typedef struct field_s {
    struct field_s * next;
    statement_declaration_t * declaration;
} field_t;

typedef struct declaration_type_base_type_s {
    bool is_primitive;
    char *identifier;
    union {
        field_t *fields;
		statement_type_declaration_t *typedef_type;
    };
} declaration_type_base_type_t;

/*!
 * TODO: add linkage type (static/extern)
 */
struct declaration_type_s
{
    declaration_type_base_t type_base;
    declaration_type_base_type_t type_base_type;
    unsigned long deref_count;
    declaration_type_modifier_t modifier;
    long enum_value;
};

struct statement_declaration
{
    declaration_type_t type;
	char * identifier;
};

struct statement_type_declaration {
    char * type_name;
    declaration_type_t type;
};

statement_declaration_t * declaration_add_indirections_identifier(
	statement_declaration_t * declaration,
	unsigned long indirections_count,
	const char * identifier
);

statement_declaration_t * create_declaration_primitive(
	char * type_identifier
);

statement_declaration_t * create_declaration_struct(
	char * struct_identifier
);

statement_type_declaration_t * create_type_declaration_struct(
	const char * struct_name,
	field_t * fields
);

statement_declaration_t * declaration_add_modifier(
	statement_declaration_t * declaration,
	declaration_type_modifier_t modifier
);

statement_type_declaration_t * type_declaration_add_modifier(
    statement_type_declaration_t * declaration,
    declaration_type_modifier_t modifier
);

field_t * declaration_create_field(
	statement_declaration_t * declaration,
	field_t * next_fields
);

#endif
