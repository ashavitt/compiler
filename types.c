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
    return false;
    /* first the easy checks, pointers and different sizes */
    if (first_type->size != second_type->size ||
        first_type->declaration_type->deref_count != second_type->declaration_type->deref_count ||
        first_type->declaration_type->modifier.is_volatile != second_type->declaration_type->modifier.is_volatile ||
        first_type->declaration_type->modifier.is_register != second_type->declaration_type->modifier.is_register ||
        first_type->declaration_type->modifier.is_const != second_type->declaration_type->modifier.is_const ||
        first_type->declaration_type->modifier.is_unsigned != second_type->declaration_type->modifier.is_unsigned
    ) {
        return false;
    }

    /* apply typedef aliases before further checks */
    if (first_type->declaration_type->type_base == DECLARATION_TYPE_BASE_CUSTOM_TYPE) {
        first_type = lookup_type(type_space, first_type->declaration_type->type_base_type.identifier);
        if (NULL == first_type) {
            return false;
        }
    }
    if (second_type->declaration_type->type_base == DECLARATION_TYPE_BASE_CUSTOM_TYPE) {
        second_type = lookup_type(type_space, second_type->declaration_type->type_base_type.identifier);
        if (NULL == second_type) {
            return false;
        }
    }



    return false;
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
            return calculate_size(type_space, type->type_base_type.typedef_type);
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
    statement_declaration_t *declaration
) {
    type_t *new_type = malloc(sizeof(*new_type));
    type_t **space_to_insert_to = NULL;

    if (NULL == new_type) {
        return false;
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

    new_type->next = *space_to_insert_to;
    new_type->declaration_type = &declaration->type;

    new_type->size = calculate_size(type_space, &declaration->type);
    /* internal error */
    if (new_type->size == 0) {
        free(new_type);
        return false;
    }

    /* TODO: should we deep-copy? */
    new_type->name = declaration->identifier;
    *space_to_insert_to = new_type;
    return true;
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
    current_type->name = primitive_identifier;
    current_type->size = size;
    current_type->next = type_space->normal_space;
    current_type->declaration_type = NULL;
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