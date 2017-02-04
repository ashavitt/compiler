%code requires {
#include "ast.h"
#include "ast_nodes.h"
}

%{
#include <stdio.h>
#include "symbol_table.h"
#include "ast.h"

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
statement_expression_t * statement_expression;
statement_declaration_t * statement_declaration;
declaration_type_e declaration_type;
}

%start file
%token TOK_CHAR TOK_INT TOK_LONG
%token <long_value> TOK_NUMBER
%token <identifier_name> TOK_IDENTIFIER

%type <statement> statement
%type <code_file> lines
%type <statement_expression> expr
%type <statement_declaration> declaration
%type <declaration_type> declaration_type

%right '='
%left "||"
%left "&&"
%left '|'
%left '^'
%left '&'
%left '+' '-'
%left '*' '/'

%%

file : lines

lines : statement ';' { code_file->first_line = $1; }
      | lines statement ';' { add_statement(code_file, $2); }

statement : expr { statement_t * stmt = malloc(sizeof(statement_t));
	  	   stmt->statement_type = EXPRESSION;
		   stmt->expression = *$1;
		   $$ = stmt; }
	  | declaration { statement_t * stmt = malloc(sizeof(statement_t));
			  stmt->statement_type = DECLARATION;
			  stmt->declaration = *$1;
			  $$ = stmt; }

expr : TOK_NUMBER { $$ = create_const_expression($1); }
     | TOK_IDENTIFIER { lookup_symbol($1);
		        $$ = create_identifier_expression($1); }
     | expr '=' expr { $$ = create_op_expression(OP_ASSIGN, $1, $3, NULL); }
     | expr "||" expr { $$ = create_op_expression(OP_OR, $1, $3, NULL); }
     | expr "&&" expr { $$ = create_op_expression(OP_AND, $1, $3, NULL); }
     | expr '|' expr { $$ = create_op_expression(OP_BOR, $1, $3, NULL); }
     | expr '^' expr { $$ = create_op_expression(OP_BXOR, $1, $3, NULL); }
     | expr '&' expr { $$ = create_op_expression(OP_BAND, $1, $3, NULL); }
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

declaration_type : TOK_CHAR { $$ = DECL_CHAR; }
		 | TOK_INT { $$ = DECL_INT; }
		 | TOK_LONG { $$ = DECL_LONG; }
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
	return 0;
}

void yyerror(code_file_t * code_file, const char * s)
{
	printf("%s\n", s);
}
