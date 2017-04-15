%code requires {
#include <ast.h>
#include <ast_nodes.h>
#include <ast_flow.h>
#include <ast_functions.h>
#include <functions.h>
}

%{
#include <stdio.h>
#include <symbol_table.h>
#include <ast.h>
#include <x86/gen_asm.h>
#include <types.h>

static int errors = 0;
static int yydebug = 0;

int yylex();
void yyerror(function_node_t * function_list, const char * s);

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

%parse-param {function_node_t * function_list}

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
statement_call_function_t * statement_call_function;
statement_call_function_param_t * statement_call_function_param;
statement_type_declaration_t * statement_type_declaration;
char * declaration_type_primitive;
declaration_type_modifier_t declaration_modifier;
unsigned long declaration_indirections;
field_t * type_declaration_struct_field_list;
function_parameter_t * function_parameters;
function_declaration_t * function_declaration;
}

%start file
%token TOK_CHAR TOK_INT TOK_LONG TOK_STRUCT
%token TOK_SIGNED TOK_UNSIGNED TOK_CONST TOK_VOLATILE TOK_REGISTER
%token TOK_IF TOK_ELSE TOK_FOR TOK_WHILE TOK_BREAK
%token TOK_EQUAL TOK_OP_OR TOK_OP_AND
%token TOK_SHIFT_LEFT TOK_SHIFT_RIGHT
%token TOK_LESS_EQUAL TOK_GREATER_EQUAL
%token TOK_TYPEDEF
%token <long_value> TOK_NUMBER
%token <identifier_name> TOK_IDENTIFIER

%type <statement> statement
%type <code_block> lines
%type <code_block> block
%type <statement_expression> expr
%type <statement_expression> optional_expr
%type <statement_declaration> declaration_with_modifier
%type <statement_type_declaration> type_declaration
%type <statement_type_declaration> type_declaration_struct
%type <statement_declaration> declaration_type
%type <statement_declaration> declaration_without_modifier
%type <statement_ifelse> ifelse
%type <statement_loop> loop
%type <statement_loop> for_loop
%type <statement_loop> while_loop
%type <statement_call_function> function_call
%type <statement_call_function_param> function_call_params
%type <declaration_type_primitive> declaration_type_primitive
%type <declaration_modifier> declaration_modifier
%type <declaration_indirections> declaration_indirections
%type <type_declaration_struct_field_list> type_declaration_struct_field_list

%type <function_parameters> function_parameters
%type <function_declaration> function_declaration

%right '='
%left TOK_OP_OR
%left TOK_OR_AND
%left '|'
%left '^'
%left '&'
%left TOK_EQUAL
%left TOK_NEQUAL
%left '<' '>'
%left TOK_GREATER_EQUAL TOK_LESS_EQUAL
%left TOK_SHIFT_RIGHT TOK_SHIFT_LEFT
%left '+' '-'
%left '*' '/'

%%

file : file function_declaration { register_new_function($2, function_list); }
     | function_declaration { register_new_function($1, function_list); }
     ;

function_declaration : declaration_without_modifier '(' function_parameters ')' block { $$ = create_function_declaration($1, $3, $5); }
		     | declaration_without_modifier '(' ')' block { $$ = create_function_declaration($1, NULL, $4); }
		     ;

function_parameters : declaration_without_modifier ',' function_parameters { $$ = add_function_parameter($3, $1); }
		    | declaration_without_modifier { $$ = add_function_parameter(NULL, $1); }
		    ;

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
	  | type_declaration ';' { $$ = create_statement_type_declaration($1); }
	  | declaration_with_modifier ';' { $$ = create_statement_declaration($1, NULL); }
	  | declaration_with_modifier '=' expr ';' { $$ = create_statement_declaration($1, $3); }
	  | ifelse { $$ = create_statement_ifelse($1); }
	  | loop { $$ = create_statement_loop($1); }
	  | TOK_BREAK ';' { $$ = create_statement_break(); }
	  | function_call ';' { $$ = create_statement_call_function($1); }
	  ;

function_call : TOK_IDENTIFIER '(' ')' { $$ = create_call_function_statement($1, NULL); }
	      | TOK_IDENTIFIER '(' function_call_params ')' { $$ = create_call_function_statement($1, $3); }
	      ;

function_call_params : expr ',' function_call_params { $$ = add_call_function_parameter($3, $1); }
		     | expr { $$ = add_call_function_parameter(NULL, $1); }
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
     | TOK_IDENTIFIER { $$ = create_identifier_expression($1); }
     | expr '=' expr { $$ = create_op_expression(OP_ASSIGN, $1, $3, NULL); }
     | expr '?' expr ':' expr { $$ = create_op_expression(OP_TERNARY, $1, $3, $5); }
     | expr TOK_OP_OR expr { $$ = create_op_expression(OP_OR, $1, $3, NULL); }
     | expr TOK_OP_AND expr { $$ = create_op_expression(OP_AND, $1, $3, NULL); }
     | expr '|' expr { $$ = create_op_expression(OP_BOR, $1, $3, NULL); }
     | expr '^' expr { $$ = create_op_expression(OP_BXOR, $1, $3, NULL); }
     | expr '&' expr { $$ = create_op_expression(OP_BAND, $1, $3, NULL); }
     | expr TOK_EQUAL expr { $$ = create_op_expression(OP_EQUAL, $1, $3, NULL); }
     | expr TOK_NEQUAL expr { $$ = create_op_expression(OP_NEQUAL, $1, $3, NULL); }
     | expr '<' expr { $$ = create_op_expression(OP_LESS, $1, $3, NULL); }
     | expr '>' expr { $$ = create_op_expression(OP_GREATER, $1, $3, NULL); }
     | expr TOK_LESS_EQUAL expr { $$ = create_op_expression(OP_LESS_EQUAL, $1, $3, NULL); }
     | expr TOK_GREATER_EQUAL expr { $$ = create_op_expression(OP_GREATER_EQUAL, $1, $3, NULL); }
     | expr TOK_SHIFT_RIGHT expr { $$ = create_op_expression(OP_BSHIFT_RIGHT, $1, $3, NULL); }
     | expr TOK_SHIFT_LEFT expr { $$ = create_op_expression(OP_BSHIFT_LEFT, $1, $3, NULL); }
     | expr '+' expr { $$ = create_op_expression(OP_ADD, $1, $3, NULL); }
     | expr '-' expr { $$ = create_op_expression(OP_SUB, $1, $3, NULL); }
     | expr '*' expr { $$ = create_op_expression(OP_MUL, $1, $3, NULL); }
     | expr '/' expr { $$ = create_op_expression(OP_DIV, $1, $3, NULL); }
     | expr '%' expr { $$ = create_op_expression(OP_MOD, $1, $3, NULL); }
     | '+' expr { $$ = create_op_expression(OP_PLUS, $2, NULL, NULL); }
     | '-' expr { $$ = create_op_expression(OP_NEG, $2, NULL, NULL); }
     | '*' expr { $$ = create_op_expression(OP_DREF, $2, NULL, NULL); }
     | '&' expr { $$ = create_op_expression(OP_REF, $2, NULL, NULL); }
     | '(' expr ')' { $$ = $2; }
     ;

/* TODO add modifier after declaration for pointer consts */
declaration_with_modifier : declaration_modifier declaration_with_modifier { $$ = declaration_add_modifier($2, $1); }
			  | declaration_without_modifier { $$ = $1; }
			  ;

declaration_without_modifier : declaration_type declaration_indirections TOK_IDENTIFIER { declaration_add_indirections_identifier($1, $2, $3); }

declaration_type : declaration_type_primitive { $$ = create_declaration_primitive($1); }
		 | TOK_STRUCT TOK_IDENTIFIER { $$ = create_declaration_struct($2); }
		 ;

type_declaration : declaration_modifier type_declaration { $$ = type_declaration_add_modifier($2, $1); }
		 | type_declaration_struct { $$ = $1; }
		 ;

type_declaration_struct : TOK_STRUCT TOK_IDENTIFIER '{' type_declaration_struct_field_list '}' { $$ = create_type_declaration_struct($2, $4); }

type_declaration_struct_field_list : type_declaration_struct_field_list declaration_with_modifier ';' { $$ = declaration_create_field($2, $1); }
				   | declaration_with_modifier ';' { $$ = declaration_create_field($1, NULL); }
				   ;

declaration_type_primitive : TOK_CHAR { $$ = "char"; }
			   | TOK_INT { $$ = "int"; }
			   | TOK_LONG { $$ = "long"; }
			   ;

declaration_indirections : declaration_indirections '*' { $$ = $1 + 1; }
			 | { $$ = 0; }
			 ;

declaration_modifier : TOK_SIGNED { $$ = (declaration_type_modifier_t) {}; }
		     | TOK_UNSIGNED { $$ = (declaration_type_modifier_t) { .is_unsigned = true}; }
		     | TOK_CONST { $$ = (declaration_type_modifier_t) { .is_const = true}; }
		     | TOK_VOLATILE { $$ = (declaration_type_modifier_t) { .is_volatile = true}; }
		     | TOK_REGISTER { $$ = (declaration_type_modifier_t) { .is_register = true}; }
		     ;

%%

extern int yylineno;

int main(int argc, char * argv[])
{
	extern FILE * yyin;

	function_node_t function_list = (function_node_t) {
		.function = NULL,
		.next = NULL
	};

	argv++;
	argc--;
	yyin = fopen(argv[0], "r");
	yydebug = 1;
	errors = 0;
	if (0 != yyparse(&function_list)) {
		return -1;
	}

	debug_ast(function_list.next);
	printf("ASSEMBLY:\n");
	if(!gen_asm_x86(function_list.next, 1)) {
		printf("\nFailed generating assembly!\n");
	} else {
		printf("\n");
	}
	fflush(stdout);
    return 0;
}

void yyerror(function_node_t * function_list, const char * s)
{
	printf("%s on line %d\n", s, yylineno);
}
