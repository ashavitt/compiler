#ifndef TYPE_H
#define TYPE_H

#include <ast_nodes.h>

typedef struct type_s {
    struct type_metadata *next;
    declaration_type_t declaration_type;
    unsigned long size;
    unsigned long alignment;
} type_t;

typedef struct type_space_s {
    type_t *primitive_space;
    type_t *struct_space;
    type_t *enum_space;
    type_t *union_space;
} type_space_t;

type_t *lookup_type(
    type_space_t *type_space,
    char * type_name
);

bool add_type(
    type_space_t *type_space,
    declaration_type_base_type_t type
);

#endif