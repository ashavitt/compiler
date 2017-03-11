#ifndef __OPCODES_H__
#define __OPCODES_H__

typedef enum opcode
{
    OPCODE_MOV,
    OPCODE_ADD,
    OPCODE_SUB,
    OPCODE_MUL,
    OPCODE_DIV,
    OPCODE_JMP
} opcode_e;

typedef enum
{
	REGISTER_EAX,
	REGISTER_EBX,
	REGISTER_ECX,
	REGISTER_EDX
} register_e;

#endif
