#include <symbol_table.h>

symbol_node_t * symbol_table = NULL;

symbol_node_t * insert_symbol(const char * name)
{
	symbol_node_t * new_node = (symbol_node_t *) malloc (sizeof(symbol_node_t *));
	char * new_node_name = (char *) malloc (strlen(name) + 1);
	if (NULL == new_node || NULL == new_node_name)
	{
		goto cleanup;
	}
	strncpy(new_node_name, name, strlen(name));
	new_node->name = new_node_name;
	new_node->next = symbol_table;
	symbol_table = new_node;
	return new_node;
cleanup:
	if (NULL != new_node)
	{
		free(new_node);
	}
	if (NULL != new_node_name)
	{
		free(new_node_name);
	}
	return NULL;
}


symbol_node_t * find_symbol(const char * name)
{
	symbol_node_t * node = symbol_table;
	while (NULL != node)
	{
		if (0 == strcmp(node->name, name))
		{
			return node;
		}
		node = node->next;
	}
	return NULL;
}
