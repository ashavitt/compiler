%{
#include <y.tab.h> /* tokens' file */
#include <string.h>
#include <stdlib.h>
%}

NUMBER			[0-9]+
IDENTIFIER		[a-zA-Z][a-zA-Z0-9_]*

%%
char			{return TOK_CHAR;}
int			{return TOK_INT;}
long			{return TOK_LONG;}
if			{return TOK_IF;}
else			{return TOK_ELSE;}
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

.			{return yytext[0];}
%%
