#include <ast_nodes.h>
#include <ast.h>
#include <stdlib.h>
#include <string.h>

statement_expression_t * create_op_expression(
	operator_type_e op,
	statement_expression_t * exp1,
	statement_expression_t * exp2,
	statement_expression_t * exp3)
{
	statement_expression_t * new_stmt = NULL;

	new_stmt = (statement_expression_t *) malloc (sizeof(statement_expression_t));
	if (NULL == new_stmt)
	{
		goto cleanup;
	}

	new_stmt->expression_type = EXPRESSION_TYPE_OP;
	new_stmt->exp_op.op = op;
	new_stmt->exp_op.exp1 = exp1;
	new_stmt->exp_op.exp2 = exp2;
	new_stmt->exp_op.exp3 = exp3;

	return new_stmt;
cleanup:
	if (NULL != new_stmt)
	{
		free(new_stmt);
	}
	// TODO add free recursion
	return NULL;
}

statement_expression_t * create_const_expression(
	long value)
{
	statement_expression_t * new_stmt = NULL;

	new_stmt = (statement_expression_t *) malloc (sizeof(statement_expression_t));
	if (NULL == new_stmt)
	{
		goto cleanup;
	}

	new_stmt->expression_type = EXPRESSION_TYPE_CONST;
	new_stmt->constant = value;

	return new_stmt;
cleanup:
	if (NULL != new_stmt)
	{
		free(new_stmt);
	}
	return NULL;
}

statement_expression_t * create_identifier_expression(
	const char * identifier)
{
	statement_expression_t * new_stmt = NULL;
	char * identifier_copy = NULL;
	size_t identifier_len = 0;

	new_stmt = (statement_expression_t *) malloc (sizeof(statement_expression_t));
	if (NULL == new_stmt)
	{
		goto cleanup;
	}

	identifier_len = strlen(identifier);
	identifier_copy = (char *) malloc (sizeof(char) * identifier_len);
	if (NULL == identifier_copy)
	{
		goto cleanup;
	}

	strncpy(identifier_copy, identifier, identifier_len);

	new_stmt->expression_type = EXPRESSION_TYPE_IDENTIFIER;
	new_stmt->identifier = identifier_copy;

	return new_stmt;
cleanup:
	if (NULL != new_stmt)
	{
		free(new_stmt);
	}
	if (NULL != identifier_copy)
	{
		free(identifier_copy);
	}
	return NULL;
}

statement_type_declaration_t * create_type_delcaration(
    unsigned long indirections_count,
    const char * identifier
) {
    statement_type_declaration_t * new_decl = NULL;
    char * identifier_copy = NULL;
    size_t identifier_len = 0;

    new_decl = (statement_type_declaration_t *) malloc (sizeof(statement_type_declaration_t));
    if (NULL == new_decl)
    {
        goto cleanup;
    }

    identifier_len = strlen(identifier);
    identifier_copy = (char *) malloc (sizeof(char) * identifier_len);
    if (NULL == identifier_copy)
    {
        goto cleanup;
    }

    strncpy(identifier_copy, identifier, identifier_len);

    new_decl->type_name = identifier_copy;

    new_decl->type.deref_count = indirections_count;
    new_decl->type.modifier = (declaration_type_modifier_t) {
            .is_const = false,
            .is_volatile = false,
            .is_unsigned = false,
            .is_register = false
    };

    return new_decl;
cleanup:
    if (NULL != new_decl)
    {
        free(new_decl);
    }
    if (NULL != identifier_copy)
    {
        free(identifier_copy);
    }
    return NULL;
}

statement_declaration_t * declaration_add_indirections_identifier(
	statement_declaration_t * declaration,
	unsigned long indirections_count,
	const char * identifier
) {
	char * identifier_copy = NULL;
	size_t identifier_len = 0;

	if (NULL == declaration)
	{
		goto cleanup;
	}

	identifier_len = strlen(identifier);
	identifier_copy = (char *) malloc (sizeof(char) * identifier_len);
	if (NULL == identifier_copy)
	{
		goto cleanup;
	}

	strncpy(identifier_copy, identifier, identifier_len);

	declaration->identifier = identifier_copy;

	declaration->type.deref_count = indirections_count;
	declaration->type.modifier = (declaration_type_modifier_t) {
		.is_const = false,
		.is_volatile = false,
		.is_unsigned = false,
		.is_register = false
	};

	return declaration;
cleanup:
	if (NULL != declaration)
	{
		free(declaration);
	}
	if (NULL != identifier_copy)
	{
		free(identifier_copy);
	}
	return NULL;
}

statement_declaration_t * create_declaration_primitive(
	char * type_identifier)
{
	statement_declaration_t * new_decl = NULL;

	new_decl = malloc(sizeof(*new_decl));
	if (NULL == new_decl)
	{
		return NULL;
	}

	new_decl->type.type_base = DECLARATION_TYPE_BASE_PRIMITIVE;
	new_decl->type.type_base_type = (declaration_type_base_type_t) {
		.is_primitive = true,
		.identifier = type_identifier
	};
	return new_decl;
}

statement_declaration_t * create_declaration_struct(
	char * struct_identifier)
{
	statement_declaration_t * new_decl = NULL;

	new_decl = malloc(sizeof(*new_decl));
	if (NULL == new_decl)
	{
		return NULL;
	}

	new_decl->type.type_base = DECLARATION_TYPE_BASE_STRUCT;
	new_decl->type.type_base_type = (declaration_type_base_type_t) {
		.is_primitive = false,
		.identifier = struct_identifier
	};
	return new_decl;
}


statement_type_declaration_t * create_type_declaration_struct(
    const char * struct_name,
    field_t * fields
) {
    statement_type_declaration_t * new_decl = NULL;

	new_decl = create_type_delcaration(0, struct_name);
	if (NULL == new_decl)
	{
		return NULL;
	}

	new_decl->type.type_base = DECLARATION_TYPE_BASE_STRUCT;
	new_decl->type.type_base_type = (declaration_type_base_type_t) {
        .identifier = (char*)struct_name,
		.is_primitive = false,
		.fields = fields
	};
	return new_decl;
}

statement_type_declaration_t * type_declaration_add_modifier(
    statement_type_declaration_t * declaration,
    declaration_type_modifier_t modifier
) {
    declaration_type_modifier_t * old_modifiers =  &declaration->type.modifier;
    old_modifiers->is_const = old_modifiers->is_const || modifier.is_const;
    old_modifiers->is_volatile = old_modifiers->is_volatile || modifier.is_volatile;
    old_modifiers->is_unsigned = old_modifiers->is_unsigned || modifier.is_unsigned;
    old_modifiers->is_register = old_modifiers->is_register || modifier.is_register;
    return declaration;
}

statement_declaration_t * declaration_add_modifier(
	statement_declaration_t * declaration,
	declaration_type_modifier_t modifier)
{
	declaration_type_modifier_t * old_modifiers =  &declaration->type.modifier;
	old_modifiers->is_const = old_modifiers->is_const || modifier.is_const;
	old_modifiers->is_volatile = old_modifiers->is_volatile || modifier.is_volatile;
	old_modifiers->is_unsigned = old_modifiers->is_unsigned || modifier.is_unsigned;
	old_modifiers->is_register = old_modifiers->is_register || modifier.is_register;
	return declaration;
}

field_t * declaration_create_field(
	statement_declaration_t * declaration,
	field_t * next_fields)
{
	field_t * new_field = malloc(sizeof(*new_field));
	if (NULL == new_field)
	{
		goto cleanup;
	}

	new_field->next = next_fields;
	new_field->declaration = declaration;

	return new_field;

cleanup:
	/* TODO free declaration */
	free(declaration);
	/* TODO free next_fields */
	return NULL;
}
