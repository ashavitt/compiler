#ifndef GEN_POINTER_H
#define GEN_POINTER_H

#include <x86/closure.h>

variable_t * pointer_allocate_variable (
	type_t * this,
	closure_t *closure,
	const char * identifier,
	value_type_e type
);

#endif