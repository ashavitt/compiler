#ifndef TYPE_H
#define TYPE_H

/* should type space be per-closure ?? */

typedef struct closure closure_t;
typedef struct type_field_s type_field_t;
typedef struct type_s type_t;
typedef struct type_field_s type_field_t;
typedef struct type_space_s type_space_t;
typedef struct variable_s variable_t;
typedef enum value_type value_type_e;

#include <ast.h>
#include <ast_nodes.h>

struct type_s {
    struct type_s *next;
    declaration_type_base_t type;
	type_field_t *fields;
	struct type_s *base_type;
	declaration_type_modifier_t modifier;
	unsigned long deref_count;
    unsigned long size;
    char * name;
	variable_t * (*allocate_variable)(
		type_t * this,
		closure_t *closure,
		const char * identifier,
		value_type_e type
	);
};

struct type_field_s {
	struct type_field_s *next_field;
	type_t *type;
	char *field_name;
};

struct type_space_s {
    type_t *normal_space;
    type_t *struct_space;
    type_t *enum_space;
    type_t *union_space;
	struct type_space_s *parent;
};

type_t *lookup_type(
    type_space_t *type_space,
    char * type_name
);

type_t * add_type(
    type_space_t *type_space,
	statement_type_declaration_t *declaration
);

type_t * add_primitive(
	type_space_t *type_space,
	char *primitive_identifier,
	unsigned long size
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

type_space_t * create_empty_type_space(type_space_t *parent);

void debug_types(type_space_t *type_space);

#endif