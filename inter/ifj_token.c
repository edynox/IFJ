/* ifj-token.c
 *
 * Copyright (C) 2016 SsYoloSwag41 Inc.
 * Author: Eduard Cuba <xcubae00@stud.fit.vutbr.cz>
 */
#include "ifj_token.h"
#include <string.h>
#include "ifj_lexa.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * Generate hashname for string constant
 * @param pointer to string
 * @return string hashname
 */
char * ifj_generate_hashname_str(char * value)
{
    int len = strlen((char *) value) + 10;
    char * hashname = (char *) malloc (sizeof(char) * len );
    snprintf(hashname, len, "//string_%s", value);
    return hashname;
}

/**
 * Generate hashname for integer constant
 * @param pointer to integer
 * @return integer hashname
 */
char * ifj_generate_hashname_int(int *value)
{
    char * hashname = (char *) malloc (sizeof(char)*16);
    snprintf(hashname, 16, "//int%d", *value);
    return hashname;
}

/**
 * Generate hashname for "for" block
 * @param for reference count
 * @return integer hashname
 */
char * ifj_generate_hashname_for(int value)
{
    char * hashname = (char *) malloc (sizeof(char)*16);
    snprintf(hashname, 16, "//for%d", value);
    return hashname;
}

/**
 * Generate hashname for double constant
 * @param pointer to double
 * @return double hashname
 */
char * ifj_generate_hashname_double(double *value)
{
    char * hashname = (char *) malloc (sizeof(char)*64);
    snprintf(hashname, 64, "//double%.16f", *value);
    return hashname;
}

/**
 * Generate symbol token
 * - if symbol is allready in symbol table, returns reference
 * - when symbol is new, save it into symbol table
 * @param table symbol table for current context
 * @param value symbol to store
 * @return reference to token
 */
token * ifj_generate_token(symbolTable *table, int type)
{
    char * hashname = (char *) malloc (sizeof(char)*3);
    if (type >= 65536)
    {
        fprintf(stderr, "Warning: symbol out of range %d\n", type);
    }
    hashname[0] = type % 256;
    hashname[1] = type / 256;
    hashname[2] = 0;
    token *item = ial_symbol_table_get_item(table, hashname, type, NULL);
    if (item)
    {
        free(hashname);
        return item;
    }
    else
    {
        item = ifj_token_new();
        item->value = NULL;
        item->type = type;
        ial_symbol_table_add_item(table, item, hashname);
        free(hashname);
        return item;
    }
}

/**
 * Generate token for constant int value
 * - if constant is allready in symbol table, returns reference
 * - when constant is new, save it into symbol table
 * @param table symbol table for current context
 * @param value value to store
 * @return reference to stored token
 */
token * ifj_generate_token_int ( symbolTable *table, int value )
{
    char *hashname = ifj_generate_hashname_int(&value);

    //get item - push table, hashname, type and hashing function for int
    token *item = ial_symbol_table_get_item(table, hashname, T_INTEGER_C, (char *(*)(void*)) &ifj_generate_hashname_int);
    if (item)
    {
        free(hashname);
        return item;
    }
    else
    {
        item = ifj_token_new();
        item->value = (void*) malloc (sizeof(int));

        *((int*) item->value) = value;
        item->type = T_INTEGER_C;

        ial_symbol_table_add_item(table, item, hashname);
        item->dataType = T_INTEGER;
        item->data = (void *)item->value;

        free(hashname);
        return item;
    }
}

/**
 * Generate token for constant double value
 * - if constant is allready in symbol table, returns reference
 * - when constant is new, save it into symbol table
 * @param table symbol table for current context
 * @param value value to store
 * @return reference to stored token
 */
token * ifj_generate_token_double ( symbolTable *table, double value )
{
    char *hashname = ifj_generate_hashname_double(&value);

    //get item - push table, hashname, type and hashing function for double
    token *item = ial_symbol_table_get_item(table, hashname, T_DOUBLE_C, (char *(*)(void*)) &ifj_generate_hashname_double);
    if (item)
    {
        free(hashname);
        return item;
    }
    else
    {
        item = ifj_token_new();
        item->value = (void*) malloc (sizeof(double));

        *((double *) item->value) = value;
        item->type = T_DOUBLE_C;

        ial_symbol_table_add_item(table, item, hashname);
        item->dataType = T_DOUBLE;
        item->data = (void *)item->value;

        free(hashname);
        return item;
    }
}

/**
 * Generate token for constant string
 * - if constant is allready in symbol table, returns reference
 * - when string is new, save it into symbol table
 * @param table symbol table for current context
 * @param value value to store
 * @return reference to stored token
 */
token * ifj_generate_token_str ( symbolTable *table, char *value )
{
    char * hashname = ifj_generate_hashname_str(value);

    //get item - push table, hashname, type and hashing function for string
    token *item = ial_symbol_table_get_item(table, hashname, T_STRING_C, (char *(*)(void*)) &ifj_generate_hashname_str);
    if (item)
    {
        free(hashname);
        return item;
    }
    else
    {
        item = ifj_token_new();
        item->value = (void *)strdup(value);

        item->type = T_STRING_C;
        ial_symbol_table_add_item(table, item, hashname);

        item->dataType = T_STRING;
        item->data = (void *)item->value;

        free(hashname);
        return item;
    }
}

/**
 * Generate token for identifier
 * - does NOT save token into symbol table
 * @param table symbol table for current context
 * @param value identifier
 * @return reference to stored token
 */
token *ifj_generate_token_id (char *value)
{
    token *item = ifj_token_new();
    item->value = (void *)strdup(value);
    item->type = T_IDENTIFIER;
    return item;
}

/**
* Generate reserved keyword for lexical analysis
* - save new keyword into symbol table
* @param table symbol table for reserved keywords
* @param value identifier
* @param type token type
* @param method token method for prebuild functions
* @return pointer to generated token
*/
token *ifj_generate_reserved (symbolTable *table, char *value, int type, int method)
{
    token *item = ial_symbol_table_get_item(table, value, type, NULL);
    if (!item)
    {
        item = ifj_token_new();
        item->value = (void *)strdup(value);

        item->type = type;
        item->method = method;
        ial_symbol_table_add_item(table, item, NULL);
    }
    return item;
}

/**
* Generate temporatu token for executor
* - token must be freed after "set" instruction
* @param dataType {T_INTEGER|T_DOUBLE|T_STRING}
* @param data for chosen dataType
* @return reference to created temporaty token
*/
token *ifj_generate_temp(int dataType, void *data)
{
    token *item = ifj_token_new();
    item->type = T_TMP;
    item->dataType = dataType;

    switch (dataType)
    {
        case T_INTEGER:
            if (!data)
            {
                item->data = NULL;
                break;
            }

            item->data = (void*) malloc (sizeof(int));
            *((int *) item->data) = *((int*) data);
            break;
        case T_DOUBLE:
            if (!data)
            {
                item->data = NULL;
                break;
            }

            item->data = (void*) malloc (sizeof(double));
            *((double *) item->data) = *((double *) data);
            break;
        case T_STRING:
            item->data = data;
            break;
    }
    return item;
}
