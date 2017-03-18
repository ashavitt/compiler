#ifndef __GEN_ASM_H__
#define __GEN_ASM_H__

#include <ast.h>
#include <stdbool.h>
#include <x86/closure.h>

typedef struct code_file_data code_file_data_t;

bool parse_block(code_block_t * code_block, closure_t * closure);
bool gen_asm_x86(code_file_t * code_file, int out_fd);

#endif
