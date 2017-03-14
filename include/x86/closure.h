#ifndef __CLOSURE__H_
#define __CLOSURE__H_

#include <x86/opcodes.h>
#include <stdlib.h>
#include <ast.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    VALUE_TYPE_VARIABLE,
    VALUE_TYPE_EXPRESSION_RESULT
} value_type_e;

typedef enum {
    OPERAND_TYPE_NONE,
    OPERAND_TYPE_REFERENCE,
    OPERAND_TYPE_REG,
    OPERAND_TYPE_STACK_BYTE,
    OPERAND_TYPE_STACK_WORD,
    OPERAND_TYPE_STACK_DWORD,
    OPERAND_TYPE_UNSIGNED_DWORD_CONST,
    OPERAND_TYPE_SIGNED_DWORD_CONST,
    OPERAND_TYPE_UNSIGNED_WORD_CONST,
    OPERAND_TYPE_SIGNED_WORD_CONST,
    OPERAND_TYPE_UNSIGNED_BYTE_CONST,
    OPERAND_TYPE_SIGNED_BYTE_CONST
} operand_type_e;

typedef struct asm_node asm_node_t;

typedef struct operand
{
    operand_type_e type;
    union {
        register_e reg;
        long stack_offset; // relative to ebp
        uint32_t unsigned_dword;
        int32_t signed_dword;
        uint16_t unsigned_word;
        int16_t signed_word;
        uint8_t unsigned_byte;
        int8_t signed_byte;
        asm_node_t * ref;
    };
} operand_e;

typedef struct {
    long stack_offset; // relative to ebp
} position_t;

typedef struct variable_s {
    struct variable_s * next;
    value_type_e type;
    position_t position;
    statement_expression_t * evaluated_expression;
    size_t size;
    union {
        char * variable_name;
    };
} variable_t;


typedef struct {
    asm_node_t * instructions;
    variable_t * variables;
} closure_t;

/* yes we use intel syntax */
struct asm_node
{
	opcode_e opcode;
    /* dst */
	operand_e operand1;
    /* src */
	operand_e operand2;
	struct asm_node * next;
};

variable_t * allocate_variable(
	closure_t * closure,
	size_t size,
	char * identifier,
	value_type_e type);

void add_instruction_to_closure(
	asm_node_t * node,
	closure_t * closure);

#endif
