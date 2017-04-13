#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__


typedef struct function_declaration_s function_declaration_t;
typedef struct function_parameter_s function_parameter_t;
typedef struct function_node_s function_node_t;

#include <ast.h>
#include <ast_nodes.h>

struct function_node_s
{
	function_declaration_t * function;
	struct function_node_s * next;
};

struct function_declaration_s
{
	char * identifier;
	declaration_type_t return_value_type;
	function_parameter_t * parameter_list;
	code_block_t * function_code;
	/* TODO add linkage modifiers */
};

struct function_parameter_s
{
	char * parameter_identifier;
	declaration_type_t parameter_type;
	struct function_parameter_s * next;
};

function_declaration_t * create_function_declaration(
	statement_declaration_t * return_value_type,
	function_parameter_t * parameter_list,
	code_block_t * function_code
);

function_parameter_t * add_function_parameter(
	function_parameter_t * previous_parameter,
	statement_declaration_t * parameter_type
);

void register_new_function(
	function_declaration_t * function_declaration,
	function_node_t * functions_list
);

function_declaration_t * lookup_function(
	char * function_name,
	function_node_t * functions_list
);

#endif

