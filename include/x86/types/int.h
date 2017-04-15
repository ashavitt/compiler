#ifndef GEN_INT_H
#define GEN_INT_H

#include <x86/closure.h>

variable_t * int_allocate_variable (
	type_t * this,
	closure_t *closure,
	const char * identifier,
	value_type_e type
);

#endif