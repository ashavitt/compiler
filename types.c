#include <types.h>
#include <stdlib.h>
#include <string.h>
#include <ast.h>
#include <stdio.h>
#include <x86/closure.h>

/* TODO: reconsider ENUM */

/* TODO: handle error return codes */
bool is_same_type(
		type_space_t *type_space,
		type_t *first_type,
		type_t *second_type
) {
	/* we assume that typedef lookup and like are done in add_type */
	if (first_type->size != second_type->size ||
		first_type->deref_count != second_type->deref_count ||
		first_type->base_type != second_type->base_type ||
		first_type->modifier.is_volatile != second_type->modifier.is_volatile ||
		first_type->modifier.is_register != second_type->modifier.is_register ||
		first_type->modifier.is_const != second_type->modifier.is_const ||
		first_type->modifier.is_unsigned != second_type->modifier.is_unsigned ||
		first_type->type != second_type->type
	) {
		return false;
	}

	return true;
}

type_t *lookup_type(
		type_space_t *type_space,
		char *type_name
) {
	type_t *current_type = NULL;

	for (current_type = type_space->normal_space; current_type != NULL; current_type = current_type->next) {
		if (0 == strcmp(current_type->name, type_name)) {
			return current_type;
		}
	}

	for (current_type = type_space->struct_space; current_type != NULL; current_type = current_type->next) {
		if (0 == strcmp(current_type->name, type_name)) {
			return current_type;
		}
	}

	for (current_type = type_space->enum_space; current_type != NULL; current_type = current_type->next) {
		if (0 == strcmp(current_type->name, type_name)) {
			return current_type;
		}
	}

	for (current_type = type_space->union_space; current_type != NULL; current_type = current_type->next) {
		if (0 == strcmp(current_type->name, type_name)) {
			return current_type;
		}
	}

	return NULL;
}

static unsigned long calculate_size(type_space_t *type_space, declaration_type_t *type);

static unsigned long align(unsigned long size) {
	return size + (size % 4 == 0 ? 0 : 4 - (size % 4));
}

static unsigned long calculate_union_size(type_space_t *type_space, declaration_type_t *type) {
	unsigned long struct_size = align(calculate_size(type_space, &type->type_base_type.fields->declaration->type));
	unsigned long current_size = 0;
	field_t *current_field = type->type_base_type.fields->next;
	while (current_field != NULL) {
		current_size = align(calculate_size(type_space, &current_field->declaration->type));
		if (current_size > struct_size) {
			struct_size = current_size;
		}
		current_field = current_field->next;
	}
	return struct_size;
}

static unsigned long calculate_struct_size(type_space_t *type_space, declaration_type_t *type) {
	unsigned long struct_size = align(calculate_size(type_space, &type->type_base_type.fields->declaration->type));
	field_t *current_field = type->type_base_type.fields->next;
	while (current_field != NULL) {
		struct_size += align(calculate_size(type_space, &current_field->declaration->type));
		current_field = current_field->next;
	}
	return struct_size;
}

static unsigned long calculate_size(type_space_t *type_space, declaration_type_t *type) {
	/* pointer is a pointer is a pointer */
	if (type->deref_count > 0) {
		return 4;
	}

	switch (type->type_base) {
		case DECLARATION_TYPE_BASE_PRIMITIVE:
			/* assuming all primitive are already aligned.. */
			return lookup_type(type_space, type->type_base_type.identifier)->size;
		case DECLARATION_TYPE_BASE_CUSTOM_TYPE:
			return calculate_size(type_space, &type->type_base_type.typedef_type->type);
		case DECLARATION_TYPE_BASE_STRUCT:
			return calculate_struct_size(type_space, type);
		case DECLARATION_TYPE_BASE_ENUM:
			return lookup_type(type_space, "int")->size;
		case DECLARATION_TYPE_BASE_UNION:
			return align(calculate_union_size(type_space, type));
	}

	return 0;
}

bool add_type(
		type_space_t *type_space,
		statement_type_declaration_t *declaration
) {
	type_t *new_type = malloc(sizeof(*new_type));
	type_t **space_to_insert_to = NULL;
	type_t *existing_type = NULL;
	field_t *fields = NULL;
	type_field_t *type_fields = NULL;

	if (NULL == new_type) {
		return false;
	}

	if (declaration->type.type_base == DECLARATION_TYPE_BASE_PRIMITIVE) {
		return false; /* WHAT THE FUCK?, we only add via add_primitive */
	}

	switch (declaration->type.type_base) {
		case DECLARATION_TYPE_BASE_STRUCT:
			space_to_insert_to = &type_space->struct_space;
			break;
		case DECLARATION_TYPE_BASE_ENUM:
			space_to_insert_to = &type_space->enum_space;
			break;
		case DECLARATION_TYPE_BASE_UNION:
			space_to_insert_to = &type_space->union_space;
			break;
		default:
			space_to_insert_to = &type_space->normal_space;
	}

	/* make sure to check for type name collisions first */
	existing_type = lookup_type(type_space, declaration->type_name);
	if (existing_type != NULL) {
		/* type already exists */
		free(new_type);
		return false;
	}

	new_type->modifier = (declaration_type_modifier_t) {
			.is_unsigned = false,
			.is_const = false,
			.is_register = false,
			.is_volatile = false
	};
	new_type->type = declaration->type.type_base;
	new_type->base_type = NULL;
	new_type->deref_count = 0;
	new_type->size = calculate_size(type_space, &declaration->type);
	/* internal error */
	if (new_type->size == 0) {
		free(new_type);
		return false;
	}

	/* TODO: should we deep-copy? */
	new_type->name = declaration->type_name;

	if ((declaration->type.type_base == DECLARATION_TYPE_BASE_UNION ||
		 declaration->type.type_base == DECLARATION_TYPE_BASE_ENUM ||
		 declaration->type.type_base == DECLARATION_TYPE_BASE_STRUCT) &&
		declaration->type.type_base_type.fields != NULL
			) {
		fields = declaration->type.type_base_type.fields;
		new_type->fields = malloc(sizeof(*new_type->fields));
		type_fields = new_type->fields;
		if (NULL == new_type->fields) {
			free(new_type);
			return false;
		}
		type_fields->type = get_declaration_type(type_space, fields->declaration);
		if (NULL == type_fields->type) {
			/* TODO: recursivly free the allocated list */
			free(new_type);
			return false;
		}
		/* TODO: should we deep-copy? */
		type_fields->field_name = fields->declaration->identifier;
		fields = fields->next;

		while (fields != NULL) {
			type_fields->next_field = malloc(sizeof(*type_fields->next_field));
			if (NULL == type_fields->next_field) {
				/* TODO: recursivly free the allocated list */
				free(new_type);
				return false;
			}
			type_fields = type_fields->next_field;
			type_fields->type = get_declaration_type(type_space, fields->declaration);
			/* TODO: should we deep-copy? */
			type_fields->field_name = fields->declaration->identifier;
			if (NULL == type_fields->type) {
				/* TODO: recursivly free the allocated list */
				free(new_type);
				return false;
			}
			fields = fields->next;
		}
	}

	/* handle typedefs */
	if (declaration->type.type_base == DECLARATION_TYPE_BASE_CUSTOM_TYPE) {
		statement_type_declaration_t *current_base_type = declaration;
		do {
			/* TODO: handle duplicate modifier */
			new_type->modifier.is_volatile &= current_base_type->type.modifier.is_volatile;
			new_type->modifier.is_register &= current_base_type->type.modifier.is_register;
			new_type->modifier.is_const &= current_base_type->type.modifier.is_const;
			new_type->modifier.is_unsigned &= current_base_type->type.modifier.is_unsigned;
			new_type->deref_count += current_base_type->type.deref_count;
			if (current_base_type->type.type_base == DECLARATION_TYPE_BASE_CUSTOM_TYPE) {
				current_base_type = current_base_type->type.type_base_type.typedef_type;
			}
		} while (current_base_type->type.type_base == DECLARATION_TYPE_BASE_CUSTOM_TYPE);

		/* now we have all the modifiers and stuff, we reached the base type */
		new_type->base_type = lookup_type(type_space, current_base_type->type_name);
		if (new_type->base_type == NULL) {
			/* we are yet to support forward declaration or types that do not exist, FTFY */
			free(new_type);
			return false;
		}
	}

	new_type->next = *space_to_insert_to;
	*space_to_insert_to = new_type;
	return true;
}

type_t *get_declaration_type(
		type_space_t *type_space,
		statement_declaration_t *declaration
) {
	/* we can just treat declaration as typedef
	 * TODO: that's really the same, we should incorparate it into our design */

	type_t *new_type = malloc(sizeof(*new_type));
	if (NULL == new_type) {
		return NULL;
	}

	new_type->modifier = declaration->type.modifier;
	new_type->deref_count = declaration->type.deref_count;
	new_type->type = declaration->type.type_base;
	new_type->base_type = NULL;
	new_type->deref_count = 0;

	/* If we have no modifiers, just return the actual type */
	if (!new_type->modifier.is_volatile &&
		!new_type->modifier.is_unsigned &&
		!new_type->modifier.is_const &&
		!new_type->modifier.is_register &&
		new_type->deref_count == 0
			) {
		return lookup_type(type_space, declaration->type.type_base_type.identifier);
	}

	/* now we have all the modifiers and stuff, we reached the base type */
	new_type->base_type = lookup_type(type_space, declaration->type.type_base_type.identifier);
	if (new_type->base_type == NULL) {
		/* we are yet to support forward declaration or types that do not exist, FTFY */
		free(new_type);
		return false;
	}

	/* apply base type modifiers, assuming that if type exists in type-space it is already looked up,
	 * it cant be typedef , the lookup is done in "add_type"
	 */
	new_type->modifier.is_register |= new_type->base_type->modifier.is_register;
	new_type->modifier.is_const |= new_type->base_type->modifier.is_const;
	new_type->modifier.is_unsigned |= new_type->base_type->modifier.is_unsigned;
	new_type->modifier.is_volatile |= new_type->base_type->modifier.is_volatile;
	new_type->deref_count += new_type->base_type->deref_count;
	/* in case we got typedef we need to update the base type */
	if (new_type->base_type->base_type != NULL) {
		new_type->base_type = new_type->base_type->base_type;
	}
	if (new_type->deref_count > 0) {
		new_type->size = 4;
	} else {
		new_type->size = new_type->base_type->size;
	}

	return new_type;
}


static bool add_primitive(
		type_space_t *type_space,
		char *primitive_identifier,
		unsigned long size
) {
	type_t *current_type = NULL;

	current_type = malloc(sizeof(*current_type));
	if (NULL == current_type) {
		return false;
	}
	/* TODO: should we deep-copy? */
	current_type->name = primitive_identifier;
	current_type->size = size;
	current_type->next = type_space->normal_space;
	current_type->base_type = NULL;
	current_type->type = DECLARATION_TYPE_BASE_PRIMITIVE;
	current_type->modifier = (declaration_type_modifier_t) {
			.is_unsigned = false,
			.is_const = false,
			.is_register = false,
			.is_volatile = false
	};
	current_type->deref_count = 0;
	type_space->normal_space = current_type;
	return true;
}

type_space_t *create_empty_type_space() {
	type_space_t *empty_space = malloc(sizeof(*empty_space));
	if (NULL == empty_space) {
		return NULL;
	}

	empty_space->struct_space = NULL;
	empty_space->enum_space = NULL;
	empty_space->union_space = NULL;

	/* TODO: free recursively */
	/* initialize primitives */
	if (!add_primitive(empty_space, "int", 4)) {
		return NULL;
	}

	if (!add_primitive(empty_space, "char", 1)) {
		return NULL;
	}

	if (!add_primitive(empty_space, "short", 2)) {
		return NULL;
	}

	if (!add_primitive(empty_space, "long long", 16)) {
		return NULL;
	}

	if (!add_primitive(empty_space, "long", 8)) {
		return NULL;
	}

	return empty_space;
}

static bool is_lvalue(
	type_space_t *type_space,
	statement_expression_t *expression
) {
	return expression->type == EXPRESSION_TYPE_IDENTIFIER /* ||
		   expression->type == EXPRESSION_TYPE_OP && expression->exp_op.op = OP_DEREF */;
}

static bool type_check_expression(
	type_space_t *type_space,
	statement_expression_t *expression,
	closure_t *closure,
	/* OUT */ type_t ** expression_type
) {
	type_t *exp1_type = NULL, *exp2_type = NULL, *exp3_type = NULL;
	variable_t *variable = NULL;
	if (EXPRESSION_TYPE_CONST == expression->type) {
		if (NULL != expression_type) {
			/* we only support int constant right now */
			*expression_type = lookup_type(type_space, "int");
		}
		return true;
	} else if (EXPRESSION_TYPE_IDENTIFIER == expression->type) {
		if (NULL != expression_type) {
			variable = get_variable(closure, expression->identifier);
			if (NULL == variable) {
				/* variable does not exist */
				return false;
			}
			*expression_type = variable->type;
		}
		return true;
	}

	/* assuming EXPRESSION_TYPE_OP */
	if (expression->type != EXPRESSION_TYPE_OP) {
		/* unhandled expression type */
		return false;
	}

	/* type check the subexpressions */
	if ((NULL != expression->exp_op.exp1) &&
		!type_check_expression(type_space, expression->exp_op.exp1, closure, &exp1_type)
	) {
		return false;
	}
	if ((NULL != expression->exp_op.exp2) &&
		!type_check_expression(type_space, expression->exp_op.exp2, closure, &exp2_type)
	) {
		return false;
	}
	if ((NULL != expression->exp_op.exp3) &&
		!type_check_expression(type_space, expression->exp_op.exp3, closure, &exp3_type)
	) {
		return false;
	}


	switch (expression->exp_op.op) {
		case OP_MUL:
		case OP_PLUS:
		case OP_DIV:
		case OP_SUB:
		case OP_ADD:
			if (NULL != expression_type) {
				*expression_type = exp1_type;
			}
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
			if (NULL != expression_type) {
				*expression_type = lookup_type(type_space, "int");
				if (NULL == *expression_type) {
					/* weird internal error */
					return false;
				}
			}
			/* we don't support conversion yet */
			return is_same_type(type_space, exp1_type, exp2_type) &&
				   (exp1_type->type == DECLARATION_TYPE_BASE_PRIMITIVE ||
				   exp1_type->deref_count > 0);
		case OP_ASSIGN:
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

	/* TODO: REMOVE THIS SHITTY HACK NIGGA, and redesign this shitty module */
	declaration_type = get_declaration_type(type_space, declaration);
	if (NULL == declaration_type) {
		return false;
	}
	/* we add variables as we type-check*/
	added_variable = allocate_variable(
		closure,
		declaration_type->size,
		declaration->identifier,
		VALUE_TYPE_VARIABLE
	);
	if (NULL == added_variable) {
		return false;
	}
	/* TODO: remove this access */
	added_variable->type = declaration_type;
	return true;
}

static bool type_check_loop(
	type_space_t *type_space,
	statement_loop_t *loop,
	closure_t *closure
) {
	type_t *condition_type = NULL;

	if ((NULL != loop->init_expression) && !type_check_expression(type_space, loop->init_expression, closure, NULL)) {
		return false;
	}
	if (!type_check_expression(type_space, loop->condition_expression, closure, &condition_type)) {
		return false;
	}
	if (
		(NULL != loop->iteration_expression) &&
		!type_check_expression(type_space, loop->iteration_expression, closure, NULL)
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
		!type_check_expression(type_space, if_statement->if_expr, closure, &if_type ) /* ||
		!type_check_block(type_space, if_statement->if_block, closure) ||
		!type_check_block(type_space, if_statement->else_block, closure) */
	) {
		return false;
	}
	/* TODO: shouldn't we have bool? */
	return is_same_type(type_space, if_type, lookup_type(type_space, "int"));
}

static bool type_check_statement(
	type_space_t *type_space,
	statement_t *statement,
	closure_t *closure
) {
	switch (statement->statement_type) {
		case STATEMENT_TYPE_DECLARATION:
			return type_check_declaration(type_space, &statement->declaration, closure);
		case STATEMENT_TYPE_TYPE_DECLARATION:
			if (!add_type(type_space, &statement->type_declaration)) {
				return false;
			}
			break;
		case STATEMENT_TYPE_LOOP:
			return type_check_loop(type_space, &statement->loop, closure);
		case STATEMENT_TYPE_IFELSE:
			return type_check_if(type_space, &statement->ifelse, closure);
		case STATEMENT_TYPE_EXPRESSION:
			return type_check_expression(type_space, &statement->expression, closure, NULL);
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

static void print_type(type_t *type) {
	printf("Type: [%s]", type->name);
	printf("\t Type's type: ");
	switch (type->type) {
		case DECLARATION_TYPE_BASE_STRUCT:
			printf("struct\n");
			break;
		case DECLARATION_TYPE_BASE_ENUM:
			printf("enum\n");
			break;
		case DECLARATION_TYPE_BASE_UNION:
			printf("union\n");
			break;
		case DECLARATION_TYPE_BASE_PRIMITIVE:
			printf("primitive\n");
			break;
		case DECLARATION_TYPE_BASE_CUSTOM_TYPE:
			printf("typedef\n");
			break;
		default:
			printf("Unknown type, internal error!\n");
	}
	printf("\tType's size: %lu\n", type->size);

	if (type->type == DECLARATION_TYPE_BASE_STRUCT ||
		type->type == DECLARATION_TYPE_BASE_UNION ||
		type->type == DECLARATION_TYPE_BASE_ENUM
			) {
		printf("\tFields start:\n");
		type_field_t *current_field = type->fields;
		while (current_field != NULL) {
			printf("\tField name: %s\t", current_field->field_name);
			print_type(current_field->type);
			current_field = current_field->next_field;
		}
		printf("\tFields end\n");
	} else if (type->type == DECLARATION_TYPE_BASE_CUSTOM_TYPE) {
		printf("\tIndirection/deref count: %lu\n", type->deref_count);
		printf("\tModifiers: ");
		if (type->modifier.is_register) {
			printf("register ");
		}
		if (type->modifier.is_const) {
			printf("const ");
		}
		if (type->modifier.is_unsigned) {
			printf("unsigned ");
		}
		if (type->modifier.is_volatile) {
			printf("volatile ");
		}
		printf("\n");
		printf("\tbase type begin:\n");
		print_type(type->base_type);
		printf("\tbase type end\n");
	}
}

void debug_types(type_space_t *type_space) {
	type_t *current_type = NULL;
	printf("Normal space:\n");
	for (current_type = type_space->normal_space; current_type != NULL; current_type = current_type->next) {
		print_type(current_type);
	}

	printf("Struct space:\n");
	for (current_type = type_space->struct_space; current_type != NULL; current_type = current_type->next) {
		print_type(current_type);
	}

	printf("Union space:\n");
	for (current_type = type_space->union_space; current_type != NULL; current_type = current_type->next) {
		print_type(current_type);
	}

	printf("Enum space:\n");
	for (current_type = type_space->enum_space; current_type != NULL; current_type = current_type->next) {
		print_type(current_type);
	}
}
