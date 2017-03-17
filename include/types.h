#ifndef TYPE_H
#define TYPE_H

#include <ast_nodes.h>

typedef struct type_metadata {
    struct type_metadata *next;
    declaration_type_t declaration_type;
    unsigned long size;
    unsigned long alignment;
} type_t;

typedef struct type_space_s {
    struct type_metadata *primitive_space;
    struct type_metadata *struct_space;
    struct type_metadata *enum_space;
    struct type_metadata *union_space;
} type_space_t;



#endif