#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#include <ast_nodes.h>

typedef struct function_declaration_s function_declaration_t;
typedef struct function_parameter_s function_parameter_t;

struct function_declaration_s
{
	char * identifier;
	declaration_type_t return_value_type;
	function_parameter_t * parameter_list;
	/* TODO add linkage modifiers */
};

struct function_parameter_s
{
	char * parameter_identifier;
	declaration_type_t parameter_type;
	struct function_parameter_s * next;
};

function_declaration_t * create_function_declaration(
	char * identifier,
	statement_declaration_t * return_value_type,
	function_parameter_t * parameter_list
);

function_parameter_t * add_function_parameter(
	function_parameter_t * previous_parameter,
	statement_declaration_t * parameter_type
);

#endif

