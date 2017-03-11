#ifndef __GEN_ASM_H__
#define __GEN_ASM_H__

#include "opcodes.h"
#include "../ast.h"

typedef struct asm_node asm_node_t;
typedef struct code_file_data code_file_data_t;

typedef union operand
{
	asm_node_t * ref;
	register_e reg;
} operand_e;

struct asm_node
{
	opcode_e opcode;
	operand_e operand1;
	operand_e operand2;
	struct asm_node * next;
};

struct code_file_data
{
	asm_node_t * first_line;
};


void gen_asm_x86(code_file_t * code_file);

#endif
