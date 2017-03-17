%code requires {
#include <ast.h>
#include <ast_nodes.h>
#include <ast_flow.h>
}

%{
#include <stdio.h>
#include <symbol_table.h>
#include <ast.h>
#include <x86/gen_asm.h>

static int errors = 0;
static int yydebug = 0;

int yylex();
void yyerror(code_file_t * code_file, const char * s);

int install_symbol(const char * symbol_name)
{
	symbol_node_t * this = NULL;
	this = find_symbol(symbol_name);
	if (NULL == this)
	{
		this = insert_symbol(symbol_name);
		if (NULL == this)
		{
			goto cleanup;
		}
	}
	else
	{
		errors++;
		printf("%s is already defined\n", symbol_name);
	}
	return 0;
cleanup:
	return -1;
}

int lookup_symbol(const char * symbol_name)
{
	if (NULL == find_symbol(symbol_name))
	{
		printf("%s is an undeclared identifier\n", symbol_name);
	}
	return 0;
}

%}

%parse-param {code_file_t * code_file}

%union {
char * identifier_name;
long long_value;
statement_t * statement;
code_file_t code_file;
code_block_t * code_block;
statement_expression_t * statement_expression;
statement_declaration_t * statement_declaration;
statement_ifelse_t * statement_ifelse;
statement_loop_t * statement_loop;
declaration_type_base_type_primitive_t declaration_type;
declaration_type_modifier_t declaration_modifier;
}

%start file
%token TOK_CHAR TOK_INT TOK_LONG
%token TOK_SIGNED TOK_UNSIGNED TOK_CONST TOK_VOLATILE TOK_REGISTER
%token TOK_IF TOK_ELSE TOK_FOR TOK_WHILE
%token TOK_EQUAL TOK_OP_OR TOK_OP_AND
%token TOK_SHIFT_LEFT TOK_SHIFT_RIGHT
%token <long_value> TOK_NUMBER
%token <identifier_name> TOK_IDENTIFIER

%type <statement> statement
%type <code_block> lines
%type <code_block> block
%type <statement_expression> expr
%type <statement_expression> optional_expr
%type <statement_declaration> declaration
%type <statement_declaration> declaration_primitive
%type <statement_ifelse> ifelse
%type <statement_loop> loop
%type <statement_loop> for_loop
%type <statement_loop> while_loop
%type <declaration_type> declaration_type
%type <declaration_modifier> declaration_modifier

%right '='
%left TOK_OP_OR
%left TOK_OR_AND
%left '|'
%left '^'
%left '&'
%left TOK_EQUAL
%left TOK_NEQUAL
%left TOK_SHIFT_RIGHT TOK_SHIFT_LEFT
%left '+' '-'
%left '*' '/'

%%

file : block { code_file->first_block = $1; }

block : '{' lines '}' { $$ = $2; }
      | statement { code_block_t * code_block = malloc(sizeof(code_block_t));
		    code_block->first_line = $1;
		    $$ = code_block; }

lines : statement { code_block_t * code_block = malloc(sizeof(code_block_t));
      		    code_block->first_line = $1;
		    $$ = code_block; }
      | lines statement { add_statement($1, $2);
			  $$ = $1; }
      ;

statement : expr ';' { $$ = create_statement_expression($1); }
	  | declaration ';' { $$ = create_statement_declaration($1); }
	  | ifelse { $$ = create_statement_ifelse($1); }
	  | loop { $$ = create_statement_loop($1); }
	  ;

ifelse : TOK_IF '(' expr ')' block TOK_ELSE block { $$ = create_ifelse_statement($3, $5, $7); }
       | TOK_IF '(' expr ')' block { $$ = create_ifelse_statement($3, $5, NULL); }
       ;

loop : for_loop
     | while_loop
     ;

for_loop : TOK_FOR '(' optional_expr ';' optional_expr ';' optional_expr ')' block { $$ = create_loop_statement($3, $5, $7, $9); }

while_loop : TOK_WHILE '(' expr ')' block  { $$ = create_loop_statement(NULL, $3, NULL, $5); }

optional_expr : expr { $$ = $1; }
	      | { $$ = NULL; }
	      ;

expr : TOK_NUMBER { $$ = create_const_expression($1); }
     | TOK_IDENTIFIER { lookup_symbol($1);
		        $$ = create_identifier_expression($1); }
     | expr '=' expr { $$ = create_op_expression(OP_ASSIGN, $1, $3, NULL); }
     | expr TOK_OP_OR expr { $$ = create_op_expression(OP_OR, $1, $3, NULL); }
     | expr TOK_OP_AND expr { $$ = create_op_expression(OP_AND, $1, $3, NULL); }
     | expr '|' expr { $$ = create_op_expression(OP_BOR, $1, $3, NULL); }
     | expr '^' expr { $$ = create_op_expression(OP_BXOR, $1, $3, NULL); }
     | expr '&' expr { $$ = create_op_expression(OP_BAND, $1, $3, NULL); }
     | expr TOK_EQUAL expr { $$ = create_op_expression(OP_EQUAL, $1, $3, NULL); }
     | expr TOK_NEQUAL expr { $$ = create_op_expression(OP_NEQUAL, $1, $3, NULL); }
     | expr TOK_SHIFT_RIGHT expr { $$ = create_op_expression(OP_BSHIFT_RIGHT, $1, $3, NULL); }
     | expr TOK_SHIFT_LEFT expr { $$ = create_op_expression(OP_BSHIFT_LEFT, $1, $3, NULL); }
     | expr '+' expr { $$ = create_op_expression(OP_ADD, $1, $3, NULL); }
     | expr '-' expr { $$ = create_op_expression(OP_SUB, $1, $3, NULL); }
     | expr '*' expr { $$ = create_op_expression(OP_MUL, $1, $3, NULL); }
     | expr '/' expr { $$ = create_op_expression(OP_DIV, $1, $3, NULL); }
     | expr '%' expr { $$ = create_op_expression(OP_MOD, $1, $3, NULL); }
     | '+' expr { $$ = create_op_expression(OP_PLUS, $2, NULL, NULL); }
     | '-' expr { $$ = create_op_expression(OP_NEG, $2, NULL, NULL); }
     | '(' expr ')' { $$ = $2; }
     ;

declaration : declaration_modifier declaration { $$ = declaration_add_modifier($2, $1); }
	    | declaration_primitive { $$ = $1; };
	    ;

declaration_primitive : declaration_type TOK_IDENTIFIER { install_symbol($2);
	    				$$ = create_declaration($1, $2); }

declaration_type : TOK_CHAR { $$ = DECLARATION_TYPE_BASE_TYPE_CHAR; }
		 | TOK_INT { $$ = DECLARATION_TYPE_BASE_TYPE_INT; }
		 | TOK_LONG { $$ = DECLARATION_TYPE_BASE_TYPE_LONG; }
		 ;

declaration_modifier : TOK_SIGNED { $$ = (declaration_type_modifier_t) {}; }
		     | TOK_UNSIGNED { $$ = (declaration_type_modifier_t) { .is_unsigned = true}; }
		     | TOK_CONST { $$ = (declaration_type_modifier_t) { .is_const = true}; }
		     | TOK_VOLATILE { $$ = (declaration_type_modifier_t) { .is_volatile = true}; }
		     | TOK_REGISTER { $$ = (declaration_type_modifier_t) { .is_register = true}; }
		     ;

%%

int main(int argc, char * argv[])
{
	extern FILE * yyin;
	code_file_t code_file = { 0 };
	argv++;
	argc--;
	yyin = fopen(argv[0], "r");
	yydebug = 1;
	errors = 0;
	yyparse(&code_file);
	debug_ast(&code_file);
	printf("ASSEMBLY:\n");
    if(!gen_asm_x86(&code_file, 1)) {
        printf("\nFailed generating assembly!\n");
    } else {
        printf("\n");
    }
    fflush(stdout);
    return 0;
}

void yyerror(code_file_t * code_file, const char * s)
{
	printf("%s\n", s);
}
