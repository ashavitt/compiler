#include <types.h>
#include <stdlib.h>
#include <string.h>
#include <ast.h>

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

static unsigned long calculate_union_size(declaration_type_t *type) {
    return 0;
}

static unsigned long calculate_struct_size(declaration_type_t *type) {
    return 0;
}

static unsigned long calculate_size(type_space_t *type_space, declaration_type_t *type) {
    /* pointer is a pointer is a pointer */
    if (type->deref_count > 0) {
        return 4;
    }

    switch(type->type_base) {
        case DECLARATION_TYPE_BASE_PRIMITIVE:
            return lookup_type(type_space, get_primitive_string(type->type_base_type.primitive))->size;
        case DECLARATION_TYPE_BASE_CUSTOM_TYPE:
            return calculate_size(type_space, type->type_base_type.typedef_type);
        case DECLARATION_TYPE_BASE_STRUCT:
            break;
        case DECLARATION_TYPE_BASE_ENUM:
            break;
        case DECLARATION_TYPE_BASE_UNION:
            break;
    }

    return 0;
}

bool add_type(
    type_space_t *type_space,
    declaration_type_t *type,
    char * type_name
) {
    type_t *new_type = malloc(sizeof(*new_type));
    type_t **space_to_insert_to = NULL;

    if (NULL == new_type) {
        return false;
    }

    switch (type->type_base) {
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
    new_type->declaration_type = type;

    new_type->size = calculate_size(type_space, type);
    /* internal error */
    if (new_type->size == 0) {
        free(new_type);
        return false;
    }

    /* align to size and to 4(that is the word size) */
    new_type->alignment = 4 - (new_type->size % 4) + new_type->size;

    /* TODO: should we deep-copy? */
    new_type->name = type_name;
    *space_to_insert_to = new_type;
    return true;
}