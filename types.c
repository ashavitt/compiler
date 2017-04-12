#include <types.h>
#include <stdlib.h>
#include <string.h>
#include <ast.h>

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
        first_type->type == second_type->type
    ) {
        return false;
    }

    return true;
}

type_t *lookup_type(
    type_space_t *type_space,
    char * type_name
) {
    type_t *current_type = NULL;

    for (current_type = type_space->normal_space; current_type!=NULL; current_type = current_type->next) {
        if (0 == strcmp(current_type->name, type_name)) {
            return current_type;
        }
    }

    for (current_type = type_space->struct_space; current_type!=NULL; current_type = current_type->next) {
        if (0 == strcmp(current_type->name, type_name)) {
            return current_type;
        }
    }

    for (current_type = type_space->enum_space; current_type!=NULL; current_type = current_type->next) {
        if (0 == strcmp(current_type->name, type_name)) {
            return current_type;
        }
    }

    for (current_type = type_space->union_space; current_type!=NULL; current_type = current_type->next) {
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

    switch(type->type_base) {
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

    new_type->modifier = (declaration_type_modifier_t){
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

		while (fields != NULL) {
			type_fields->next_field = malloc(sizeof(*type_fields->next_field));
			if (NULL == type_fields->next_field) {
				/* TODO: recursivly free the allocated list */
				free(new_type);
				return false;
			}
			type_fields = type_fields->next_field;
			type_fields->type = get_declaration_type(type_space, fields->declaration);
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

type_t * get_declaration_type(
	type_space_t *type_space,
	statement_declaration_t *declaration
) {
	/* we can just treat declaration as typedef
	 * TODO: that's really the same, we should incorparate it into our design */
	type_t *new_type = malloc(sizeof(*new_type));
	if (NULL == new_type) {
		return NULL;
	}

	new_type->modifier = (declaration_type_modifier_t){
			.is_unsigned = false,
			.is_const = false,
			.is_register = false,
			.is_volatile = false
	};
	new_type->type = declaration->type.type_base;
	new_type->base_type = NULL;
	new_type->deref_count = 0;
	new_type->size = calculate_size(type_space, &declaration->type);

	statement_type_declaration_t real_declaration_type = {
		.type_name = declaration->type.type_base_type.identifier,
		.type = declaration->type
	};
	statement_type_declaration_t *current_base_type = &real_declaration_type;
	do {
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

	return new_type;
}


static bool add_primitive(
    type_space_t *type_space,
    char * primitive_identifier,
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
    current_type->modifier = (declaration_type_modifier_t){
        .is_unsigned = false,
        .is_const = false,
        .is_register = false,
        .is_volatile = false
    };
    current_type->deref_count = 0;
    type_space->normal_space = current_type;
    return true;
}

type_space_t * create_empty_type_space() {
    type_space_t *empty_space = malloc(sizeof(*empty_space));
    if (NULL == empty_space) {
        return NULL;
    }

    empty_space->struct_space = NULL;
    empty_space->enum_space = NULL;
    empty_space->union_space = NULL;

    /* TODO: free recursively */
    /* initialize primitives */
    if(!add_primitive(empty_space, "int", 4)) {
        return NULL;
    }

    if(!add_primitive(empty_space, "char", 1)) {
        return NULL;
    }

    if(!add_primitive(empty_space, "short", 2)) {
        return NULL;
    }

    if(!add_primitive(empty_space, "long long", 16)) {
        return NULL;
    }

    if(!add_primitive(empty_space, "long", 8)) {
        return NULL;
    }

    return empty_space;
}

bool type_check(type_space_t *type_space, code_file_t *code_file) {
	statement_t *current_statement = code_file->first_block->first_line;
	while (current_statement != NULL) {
		if (current_statement->statement_type == STATEMENT_TYPE_DECLARATION) {
			/* TODO: REMOVE THIS SHITTY HACK NIGGA, and redesign this shitty module */
			type_t * declaration_type = get_declaration_type(type_space, &current_statement->declaration);
			if (NULL == declaration_type) {
				return false;
			}
		}
		else if (current_statement->statement_type == STATEMENT_TYPE_TYPE_DECLARATION) {
			if (!add_type(type_space, &current_statement->type_declaration)) {
				return false;
			}
		}
		current_statement = current_statement->next;
	}

	return true;
}
