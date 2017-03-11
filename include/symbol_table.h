#ifndef _SYMBOL_TABLE_H_
#define _SYMBOL_TABLE_H_

#include <stdlib.h>
#include <string.h>

typedef struct symbol_node
{
	const char * name;
	unsigned int flags;
	struct symbol_node * next;
} symbol_node_t;

extern symbol_node_t * symbol_table;
symbol_node_t * insert_symbol(const char * name);
symbol_node_t * find_symbol(const char * name);

#endif
