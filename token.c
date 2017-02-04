#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

long int number_value = 0;

enum c_token {
	tok_invalid = -1,
	tok_eof = 0,

	tok_identifier = 0x100,
	tok_number = 0x101,

	tok_char = 0x200,
	tok_int = 0x201,
	tok_long = 0x202
};

static enum c_token get_tok()
{
	static int last_char = ' ';
	char identifier_str[63] = {0};
	unsigned char identifier_str_index = 0;
	char number_str[63] = {0};
	unsigned char number_str_index = 0;
	while (isspace(last_char))
	{
		last_char = getchar();
	}
	// identifier
	if (isalpha(last_char) || '_' == last_char)
	{
		identifier_str[identifier_str_index] = last_char;
		identifier_str_index++;
		last_char = getchar();
		// TODO buffer overflow
		while (isalnum(last_char) || '_' == last_char)
		{
			identifier_str[identifier_str_index] = last_char;
			identifier_str_index++;
			last_char = getchar();
		}
		identifier_str[identifier_str_index] = '\0';
		if (0 == strcmp(identifier_str, "char"))
		{
			return tok_char;
		}
		if (0 == strcmp(identifier_str, "int"))
		{
			return tok_int;
		}
		if (0 == strcmp(identifier_str, "long"))
		{
			return tok_long;
		}
		return tok_identifier;
	}
	// number
	if (isdigit(last_char))
	{
		do {
			number_str[number_str_index] = last_char;
			number_str_index++;
			last_char = getchar();
		} while (isdigit(last_char));
		// TODO error handling
		number_str[number_str_index] = '\0';
		number_value = strtol(number_str, NULL, 0);
		return tok_number;
	}
	// eof
	if (EOF == last_char)
	{
		return tok_eof;
	}
	return tok_invalid;
}

int main()
{
	enum c_token tok = tok_invalid;
	while (tok_eof != tok)
	{
		tok = get_tok();
		switch(tok)
		{
			case tok_number:
				printf("number\n");
				break;
			case tok_identifier:
				printf("identifier\n");
				break;
			case tok_char:
				printf("char\n");
				break;
			case tok_int:
				printf("int\n");
				break;
			case tok_long:
				printf("long\n");
				break;
			case tok_eof:
				printf("eof\n");
				break;
			case tok_invalid:
				printf("invalid\n");
				break;
		}
	}
	return 0;
}

