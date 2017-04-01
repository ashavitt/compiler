%{
#include <y.tab.h> /* tokens' file */
#include <string.h>
#include <stdlib.h>
%}

NUMBER			(0x)?[0-9]+
IDENTIFIER		[a-zA-Z_][a-zA-Z0-9_]*

%%
char			{return TOK_CHAR;}
int			{return TOK_INT;}
long			{return TOK_LONG;}
signed			{return TOK_SIGNED;}
unsigend		{return TOK_UNSIGNED;}
const			{return TOK_CONST;}
volatile		{return TOK_VOLATILE;}
register		{return TOK_REGISTER;}
struct			{return TOK_STRUCT;}
if			{return TOK_IF;}
else			{return TOK_ELSE;}
for			{return TOK_FOR;}
while			{return TOK_WHILE;}
break			{return TOK_BREAK;}
{IDENTIFIER}		{
	yylval.identifier_name = strdup(yytext);
	return TOK_IDENTIFIER;}
{NUMBER}		{
	yylval.long_value = strtol(yytext, NULL, 0);
	return TOK_NUMBER;}
[ \t\r\n]+	

==			{ return TOK_EQUAL; }
!=			{ return TOK_NEQUAL; }
\|\|			{ return TOK_OP_OR; }
&&			{ return TOK_OP_AND; }
\>\>			{ return TOK_SHIFT_RIGHT; }
\<\<			{ return TOK_SHIFT_LEFT; }
\<=			{ return TOK_LESS_EQUAL; }
\>=			{ return TOK_GREATER_EQUAL; }

.			{return yytext[0];}
%%
