#include <x86/types/pointer.h>
#include <x86/gen_asm.h>
#include <x86/closure.h>
#include <types/type_check.h>
#include <string.h>
//
///* TODO: load and stores of registers should probably be in closure */
//static bool load_ptr_from_stack(variable_t * variable, closure_t * closure, register_e target_register) {
//	asm_node_t * node = NULL;
//
//	if (NULL == variable) {
//		return false;
//	}
//
//	node = malloc(sizeof(*node));
//	if (NULL == node) {
//		return false;
//	}
//
//	node->operand1.type = OPERAND_TYPE_REG;
//	node->operand1.reg = target_register;
//	node->operand2.type = OPERAND_TYPE_STACK_DWORD;
//	node->operand2.stack_offset = variable->position.stack_offset;
//
//	add_instruction_to_closure(node, closure);
//	return true;
//}
//
//static bool store_register_to_variable(variable_t * variable, register_e source_register, closure_t * closure) {
//	asm_node_t * node_store = malloc(sizeof(*node_store));
//
//	if (NULL == node_store) {
//		return false;
//	}
//
//	node_store->opcode = OPCODE_MOV;
//	node_store->operand1.type = OPERAND_TYPE_STACK_DWORD;
//	node_store->operand1.stack_offset = variable->position.stack_offset;
//	node_store->operand2.type = OPERAND_TYPE_REG;
//	node_store->operand2.reg = source_register;
//	add_instruction_to_closure(node_store, closure);
//	return true;
//}
//
//static bool generate_ptr_deref(
//	statement_expression_t * operation,
//	closure_t * closure,
//	type_space_t *type_space
//) {
//	variable_t * ptr_variable = NULL;
//	variable_t * result = NULL;
//
//	if (!generate_expression(operation->exp_op.exp1, closure, type_space)) {
//		return false;
//	}
//
//	ptr_variable = lookup_expression_result(operation->exp_op.exp1);
//	if (NULL == ptr_variable) {
//		return false;
//	}
//
//	/* this is actually divided to two cases, the first we deref rvalue pointer and the second we deref lvalue pointer*/
//	if (is_lvalue(type_space, operation->exp_op.exp1)) {
//
//	}
//	else {
//
//	}
//	result = allocate_variable(closure, NULL, VALUE_TYPE_VARIABLE);
//}
//
//static bool generate_ptr_ref(
//	statement_expression_t * operation,
//	closure_t * closure,
//	type_space_t *type_space
//) {
//	return false;
//}
//
//static bool ptr_generate_operation(
//	statement_expression_t * operation,
//	closure_t * closure,
//	type_space_t *type_space
//) {
//	switch (expression->exp_op.op) {
//		case OP_ASSIGN:
//			break;
//		case OP_DREF:
//			return generate_ptr_deref(operation, closure, type_space);
//		case OP_REF:
//			return generate_ptr_ref(operation, closure, type_space);
//		case OP_ADD:
//			break; /* TODO: add support for this in the type-checker first */
//	}
//	return false;
//}
//
//variable_operations_t ptr_operations = {
//	.generate_operation = &ptr_generate_operation
//};
//
//variable_t * pointer_allocate_variable (
//	type_t * this,
//	closure_t *closure,
//	const char * identifier,
//	value_type_e type
//) {
//	variable_t * ptr_var = allocate_variable(closure, identifier, type, this);
//	if (NULL == ptr_var) {
//		return NULL;
//	}
//
//	ptr_var->op = &ptr_operations;
//	return int_var;
//}