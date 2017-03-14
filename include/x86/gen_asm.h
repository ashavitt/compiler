#ifndef __GEN_ASM_H__
#define __GEN_ASM_H__

#include <ast.h>
#include <stdbool.h>

typedef struct code_file_data code_file_data_t;

bool gen_asm_x86(code_file_t * code_file, int out_fd);

#endif
