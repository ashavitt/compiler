#ifndef TYPE_H
#define TYPE_H

#include <ast_nodes.h>

typedef struct type_metadata {
    declaration_type_t declaration_type;
    unsigned long size;
    unsigned long alignment;
    union {

    };
} type_t;

#endif