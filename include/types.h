#ifndef TYPE_H
#define TYPE_H

#include <ast.h>
#include <ast_nodes.h>

/* should type space be per-closure ?? */

typedef struct closure closure_t;
typedef struct type_field_s type_field_t;

typedef struct type_s {
    struct type_s *next;
    declaration_type_base_t type;
	type_field_t *fields;
	struct type_s *base_type;
	declaration_type_modifier_t modifier;
	unsigned long deref_count;
    unsigned long size;
    char * name;
} type_t;

typedef struct type_field_s {
	struct type_field_s *next_field;
	type_t *type;
	char *field_name;
} type_field_t;

typedef struct type_space_s {
    type_t *normal_space;
    type_t *struct_space;
    type_t *enum_space;
    type_t *union_space;
	struct type_space_s *parent;
} type_space_t;

type_t *lookup_type(
    type_space_t *type_space,
    char * type_name
);

bool add_type(
    type_space_t *type_space,
	statement_type_declaration_t *declaration
);

type_t * get_declaration_type(
	type_space_t *type_space,
	statement_declaration_t *declaration
);

bool is_same_type(
    type_space_t *type_space,
    type_t *first_type,
    type_t *second_type
);

bool type_check_expression(
	type_space_t *type_space,
	statement_expression_t *expression,
	closure_t *closure,
	/* OUT */ type_t ** expression_type
);

type_space_t * create_empty_type_space(type_space_t *parent);

bool type_check_block(type_space_t *type_space, code_block_t *code_block, closure_t *closure);

void debug_types(type_space_t *type_space);

#endif