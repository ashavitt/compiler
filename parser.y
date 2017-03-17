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
declaration_type_t declaration_type;
}

%start file
%token TOK_CHAR TOK_INT TOK_LONG
%token TOK_SIGNED TOK_UNSIGNED
%token TOK_IF TOK_ELSE
%token TOK_EQUAL TOK_OP_OR TOK_OP_AND
%token TOK_SHIFT_LEFT TOK_SHIFT_RIGHT
%token <long_value> TOK_NUMBER
%token <identifier_name> TOK_IDENTIFIER

%type <statement> statement
%type <code_block> lines
%type <code_block> block
%type <statement_expression> expr
%type <statement_declaration> declaration
%type <statement_ifelse> ifelse
%type <declaration_type> declaration_type

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
	  ;

ifelse : TOK_IF '(' expr ')' block TOK_ELSE block { $$ = create_ifelse_statement($3, $5, $7); }
       | TOK_IF '(' expr ')' block { $$ = create_ifelse_statement($3, $5, NULL); }
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

declaration : declaration_type TOK_IDENTIFIER { install_symbol($2);
	    				$$ = create_declaration($1, $2); }

declaration_type : TOK_CHAR { $$ = DECLARATION_TYPE_BASE_TYPE_CHAR; }
		 | TOK_INT { $$ = DECLARATION_TYPE_BASE_TYPE_INT; }
		 | TOK_LONG { $$ = DECLARATION_TYPE_BASE_TYPE_LONG; }
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
