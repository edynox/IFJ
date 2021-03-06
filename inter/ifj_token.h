/* ifj-token.h
 *
 * Copyright (C) 2016 SsYoloSwag41 Inc.
 * Author: Eduard Cuba <xcubae00@stud.fit.vutbr.cz>
 */

#ifndef IFJ_UTILS_H
#define IFJ_UTILS_H

#include "ifj_inter.h"

char * ifj_generate_hashname ( token *item);

token * ifj_generate_token(symbolTable *table, int type);

token * ifj_generate_token_int ( symbolTable *table, int value );

token * ifj_generate_token_double ( symbolTable *table, double value );

token * ifj_generate_token_str ( symbolTable *table, char *value);

token * ifj_generate_token_id (char *value);
char * ifj_generate_hashname_for(int value);

token *ifj_generate_reserved (symbolTable *table, char *value, int type, int method);

char * ifj_generate_hashname_str(char * value);
char * ifj_generate_hashname_int(int *value);
char * ifj_generate_hashname_double(double *value);
token *ifj_generate_temp(int dataType, void *data);

#endif
