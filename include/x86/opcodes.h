#ifndef __OPCODES_H__
#define __OPCODES_H__

typedef enum opcode
{
    OPCODE_MOV,
    OPCODE_ADD,
    OPCODE_SUB,
    OPCODE_MUL,
    OPCODE_IMUL,
    OPCODE_DIV,
    OPCODE_XOR,
    OPCODE_AND,
    OPCODE_OR,
    OPCODE_JMP,
    OPCODE_PUSH,
    OPCODE_POP
} opcode_e;

typedef enum
{
	REGISTER_EAX,
	REGISTER_EBX,
	REGISTER_ECX,
	REGISTER_EDX,
	REGISTER_EDI,
	REGISTER_ESI
} register_e;

#endif
