#ifndef __OPCODES_H__
#define __OPCODES_H__

typedef enum opcode
{
	ADD,
	SUB,
	MUL,
	DIV,
	JMP
} opcode_e;

typedef enum registr
{
	EAX,
	EBX,
	ECX,
	EDX
} register_e;

#endif
