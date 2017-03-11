#include "gen_asm.h"
#include <stdlib.h>

asm_node_t * parse_block(code_block_t * code_block, code_file_data_t * code_file_data)
{
}

void gen_asm_x86(code_file_t * code_file)
{
	code_file_data_t code_file_data = { 0 };
	code_block_t * code_block = code_file->first_block;
	asm_node_t asm_code = { 0 };
	asm_node_t * next_asm_code = NULL;
	while (code_block != NULL)
	{
		next_asm_code = parse_block(code_block, &code_file_data);
		code_block = NULL; // TODO add the other blocks
	}
}
