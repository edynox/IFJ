/* ifj-syna.c
*
* Copyright (C) 2016 SsYoloSwag41 Inc.
* Authors: Jan Demcak <xdemca01@stud.fit.vutbr.cz>
*/

#include "ifj-syna.h"
#include "ifj-inter.h"

//rules
//terminals
//nonterminals

/**
* Run syntactic analysis
* @return 1 when successful
*/

/*TODO JANY funcia run je bezparametricka staticka   */
/*TODO JANY ak je void funkcia tak  nesmie obsahovat return*/
/*TODO JANNY Každá funkce vrací hodnotu danou vyhodnocením výrazu v příkazu return . V pří-
padě chybějící návratové hodnoty kvůli neprovedení příkazu return dojde k chybě 8.*/
/* TODO JANNY Nedošlo-li k vykonání žádného příkazu return a nejedná se o void -funkci, nastává
běhová chyba 8. */
/*TODO JANY Pokus o vytvoření uživa-
telské třídy ifj16 je chyba 3*/

/* TODO JANY Pokus o přiřazení návratové hodnoty z void -funkce vede na chybu 8. */
/*TODO JANNY Pokud je vyhodnocený výraz pravdivý,
vykoná se složený_příkaz 1 , jinak se vykoná složený_příkaz 2 . Pokud výsledná hod-
nota výrazu není pravdivostní (tj. pravda či nepravda), nastává chyba 4. */
/*TODO JANY  opravit navratove kody
1 - chyba v programu v rámci lexikální analýzy (chybná struktura aktuálního lexé-
mu).
• 2 - chyba v programu v rámci syntaktické analýzy (chybná syntaxe programu).
• 3 - sémantická chyba v programu – nedefinovaná třída/funkce/proměnná, pokus o re-
definici třídy/funkce/proměnné, atd. ak chyba funkcia RUN
• 4 - sémantická chyba typové kompatibility v aritmetických, řetězcových a relačních
výrazech, příp. špatný počet či typ parametrů u volání funkce.
• 6 - ostatní sémantické chyby.
• 7 - běhová chyba při načítání číselné hodnoty ze vstupu.
• 8 - běhová chyba při práci s neinicializovanou proměnnou.
• 9 - běhová chyba dělení nulou.
• 10 - ostatní běhové chyby.
• 99 - interní chyba interpretu tj. neovlivněná vstupním programem (např. chyba a-
lokace paměti, chyba při otvírání souboru s řídicím programem, špatné parametry
příkazové řádky atd.).*/

/**
 * Next class or EOF
 * @param self global structure
 * @return 1 if EOF or some proper class definition, 0 when some garbage found
 **/
int next_class(ifjInter *self)
{
    token * active = lexa_next_token(self->lexa_module, self->table);
    if ((active->type) == T_END)
    {
        return 1;
    }
    else if (active->type == T_CLASS && is_ID(self, self->table, &active, 1))
    {
        if(self->preLoad)
        {
            active->childTable = ial_symbol_table_new();
            active->childTable->parent = self->table;
        }
        if(active->childTable)
        return class_inside(self, active->childTable) &&
               next_class(self);
    }

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * New class beginning with {
 * @param self global structure
 * @param table symbol table for current class
 * @return 0 is successfull
 **/
int class_inside(ifjInter *self, symbolTable *table)
{
    token * active = lexa_next_token(self->lexa_module, self->table);
    if (active->type == T_LBLOCK)
    {
        return class_inside1(self, table);
    }

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * Inside class, static function, static variable or } expected
 * @param self global structure
 * @param table symbol table for current class
 * @return 1 if successfull
 **/
int class_inside1(ifjInter *self, symbolTable *table)
{
    token * active = lexa_next_token(self->lexa_module, self->table);
    if (active->type != T_RBLOCK)
    {
        if (active->type == T_STATIC)
        {
            return get_type_with_void(self, &active) &&
                   is_ID(self, table, &active, 1) &&
                   class_inside2(self, table, active);
        }
        else
        {
            print_unexpected(self, active);
            self->returnCode = 2;
            return 0;
        }
    }

    return 1;
}

/**
 * Fetch next token, assert idenfifier type, set datatype
 * @param self global structure
 * @param item reference to last token, reference to new token returned
 * @param stat 1 if idenfifier is static, else 0
 * @return 1 if successfull
 **/
int is_ID(ifjInter *self, symbolTable *table, token **item, int stat)
{
    token *active = lexa_next_token(self->lexa_module, table);
    token *prev = *item;
    *item = active;
    int flag = !stat || (stat && self->preLoad);
    if (active->type == T_IDENTIFIER)
    {
        switch (prev->type)
        {
            //declarations
            case T_CLASS:
                active->dataType = T_CLASS;
                return resolve_identifier(self, table, item, flag);

            case T_INTEGER:
                active->dataType = T_INTEGER;
                return resolve_identifier(self, table, item, flag);

            case T_DOUBLE:
                active->dataType = T_DOUBLE;
                return resolve_identifier(self, table, item, flag);

            case T_STRING:
                active->dataType = T_STRING;
                return resolve_identifier(self, table, item, flag);

            case T_VOID:
                active->dataType = T_VOID;
                return resolve_identifier(self, table, item, flag);

            //not a declaration
            default:
                return resolve_identifier(self, table, item, 0);

        }
    }

    print_unexpected(self, active);
    self->returnCode = 4;
    return 0;
}

/**
 * Resolving args for called functions. Fetch next token, expects "arg" or ")"
 * @param self global structure
 * @param table symbol table for current function (not called one!)
 * @param expected prototype of expected argument for type check
 * @return 1 if successfull
 **/
int next_param(ifjInter *self, symbolTable *table, token *expected)
{
    token * active = lexa_next_token(self->lexa_module, table);
    if (active->type == T_IDENTIFIER)
    {
        int rc = resolve_identifier(self, table, &active, 0);
        if(!rc)
        {
            return rc;
        }
        if(expected && check_typing(active, expected))
        {
            return 1;
        }
        print_mistyped(self, active, expected);
        self->returnCode = 4;
        return 4;
    }
    else if(active->type == T_INTEGER_C ||
            active->type == T_DOUBLE_C ||
            active->type == T_STRING_C)
    {
        if(expected && check_typing(active, expected))
        {
            return 1;
        }
        print_mistyped(self, active, expected);
        self->returnCode = 4;
        return 0;
    }

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * Skip code until next semicolon - used when NOT in preLoad
 * @param self global structure
 **/
void skip_to_semicolon(ifjInter *self)
{
    token * active = lexa_next_token(self->lexa_module, self->table);
    while (active->type != T_SEMICOLON)
    {
        if(active->type == T_IDENTIFIER)
        {
            ifj_token_free(active);
        }
        active = lexa_next_token(self->lexa_module, self->table);
    }
}

/**
 * Skip code until next rblock ("}") - used when in preLoad
 * @param self global structure
 * @return 1 if rblock found else 0
 **/
int skip_to_rblock(ifjInter *self)
{
    token * active = lexa_next_token(self->lexa_module, self->table);
    int rc;
    while(active->type != T_RBLOCK && active->type != T_END)
    {
        //when another sub-block found, skip to its "}" recursively
        if(active->type == T_IDENTIFIER)
        {
            ifj_token_free(active);
        }
        else if(active->type == T_LBLOCK)
        {
            rc = skip_to_rblock(self);
            if(!rc)
                return rc;
        }
        active = lexa_next_token(self->lexa_module, self->table);
    }
    if(active->type == T_END)
    {
        print_unexpected(self, active);
        self->returnCode = 2;
        return 0;
    }
    return 1;
}

/**
 * Fetch next token. Now we are in state "type identifier". We expect "(" for
 * function declaration, ";" or "=" for variable declaration
 * @param self global structure
 * @param table symbol table for current class
 * @param item last token / identifier
 * @return 1 if successfull
 **/
int class_inside2(ifjInter *self, symbolTable *table, token *item)
{
    token * active = lexa_next_token(self->lexa_module, self->table);
    //function
    if (active->type == T_LPAREN)
    {
        if(self->preLoad)
        {
            item->childTable = ial_symbol_table_new();
            item->childTable->parent = table;
        }
        return function_declar(self, item) &&
               function_inside(self, item) &&
               class_inside1(self, table);
    }
    //variable
    else if (active->type == T_SEMICOLON)
    {
        //TODO check if not void here
        return class_inside1(self, table);
    }
    else if (active->type == T_ASSIGN)
    {
        if(self->preLoad)
        {
            //TODO check if not void here
            //TODO initialize static variable here
            return expresion(self, table) && class_inside1(self, table);
        }
        else
        {
            skip_to_semicolon(self);
            return class_inside1(self, table);
        }
    }
    //some garbage

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * Fetch next token. Expects type definition (int, double, String, void)
 * @param self global structure
 * @param item returns reference to fetched token
 * @return 1 if successfull
 **/
int get_type_with_void(ifjInter *self, token **item)
{
    token * active = lexa_next_token(self->lexa_module, self->table);
    *item = active;
    switch(active->type)
    {
        case T_INTEGER:
        case T_DOUBLE:
        case T_STRING:
        case T_VOID:
            return 1;
    }

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * Fetch next token. Expects type definition (int, double, String)
 * @param self global structure
 * @param item returns reference to fetched token
 * @return 1 if successfull
 **/
int get_type_without_void(ifjInter *self, token **item)
{
    token * active = lexa_next_token(self->lexa_module, self->table);
    *item = active;
    switch(active->type)
    {
        case T_INTEGER:
        case T_DOUBLE:
        case T_STRING:
            return 1;
    }

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * Fetch first argument for function declaration ("type identifier" or ")")
 * - save arguments in child table
 * - set args stack
 * @param self global structure
 * @param item function token
 * @return 1 if successfull
 **/
int function_declar(ifjInter *self, token *item)
{
    token * active = lexa_next_token(self->lexa_module, item->childTable);
    switch(active->type)
    {
        case T_INTEGER:
        case T_DOUBLE:
        case T_STRING:
            if(is_ID(self, item->childTable, &active, 1))
            {
                if(self->preLoad)
                {
                    item->args = ifj_stack_new();
                    ifj_stack_push(item->args, active);
                }
                return next_function_param(self, item);
            }
            else
            {
                print_unexpected(self, active);
                self->returnCode = 2;
                return 0;
            }

        case T_RPAREN:
            return 1;
    }

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * Fetch n-th argument for function declaration (")", "type identifier" or ",")
 * - save arguments in child table
 * - set args stack
 * @param self global structure
 * @param item function token
 * @return 1 if successfull
 **/
int next_function_param(ifjInter *self, token *item)
{
    token * active = lexa_next_token(self->lexa_module, self->table);
    if (active->type == T_COMMA)
    {
        if(get_type_without_void(self, &active) &&
           is_ID(self, item->childTable, &active, 1))
        {
            if(self->preLoad)
            {
                ifj_stack_push(item->args, active);
            }
            return next_function_param(self, item);
        }
        else
        {
            print_unexpected(self, active);
            self->returnCode = 2;
            return 0;
        }
    }
    else if(active->type != T_RPAREN)
    {
        print_unexpected(self, active);
        self->returnCode = 2;
        return 0;
    }

    return 1;
}

/**
 * Fetch next token. Expects "{".
 * Skips until "}" found when preLoad active
 * @param self global structure
 * @param current function token
 * @return 1 if successfull
 **/
int function_inside(ifjInter *self, token *item)
{
    token * active = lexa_next_token(self->lexa_module, self->table);
    if(self->preLoad)
    {
        return skip_to_rblock(self);
    }
    if(active->type == T_LBLOCK)
    {
        return function_inside1(self, item);
    }

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}


/**
 * Inside function. Expects "}", "while", "break", "continue", "if", "return",
 * "int", "double", "String", "identifier"
 * FIXME: I dont think, that we can accept "break" or "continue" here
 * @param self global structure
 * @param item current function token
 * @return 1 if successfull
 **/
int function_inside1(ifjInter *self, token *item)
{
    token * active = NULL;
    if(self->pushBack)
    {
        active = self->pushBack;
        self->pushBack = NULL;
    }
    else
    {
        active = lexa_next_token(self->lexa_module, item->childTable);
    }

    switch (active->type)
    {
        case T_RBLOCK:
            return 1;

        case T_WHILE:
            return condition(self, item->childTable) &&
                   statement_inside1(self, item->childTable) &&
                   function_inside1(self, item);

      /*  case T_FOR:
        return is_LPAREN(self) &&
               tell_me_type_without_void(self) &&
               is_ID(self) &&
               is_ASSIGN(self) &&
               expresion(self) &&
               is_semicolon(self) &&
               condition(self) &&
               is_semicolon(self) &&
               is_ID(self) &&
               is_ASSIGN(self) &&
               expresion(self)&&
               is_RPAREN(self) &&
               statement_inside(self) &&
               function_inside1(self);*/

      /*  case T_DO:
        return statement_inside(self) &&
               is_while(self) &&
               is_LPAREN(self) &&
               condition(self) &&
               is_RPAREN(self) &&
               is_semicolon(self) &&
               function_inside1(self));*/
            
        //TODO generate jump + else jump
        case T_IF:
            return condition(self, item->childTable) &&
                   statement_inside1(self, item->childTable) &&
                   if_else1(self, item->childTable) &&
                   function_inside1(self, item);

        //TODO generate jump
        case T_RETURN:
            return expresion(self, item->childTable) &&
                   statement_inside1(self, item->childTable);

        case T_INTEGER:
        case T_STRING:
        case T_DOUBLE:
            return is_ID(self, item->childTable, &active, 0) &&
                   sth_next(self, item->childTable, active) &&
                   function_inside1(self, item);

        case T_IDENTIFIER:
        {
            int rc;
            rc = resolve_identifier(self, item->childTable, &active, 0);

            if(rc)
            {
                return fce(self, item->childTable, active) &&
                       function_inside1(self, item);
            }

            return rc;
        }
    }

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * Fetch next token, expects semicolon
 * @param self global structure
 * @return 1 if successfull
 **/
int is_semicolon(ifjInter *self)
{
    token * active = lexa_next_token(self->lexa_module, self->table);
    if (active->type != T_SEMICOLON)
    {
        print_unexpected(self, active);
        self->returnCode = 2;
        return 0;
    }

    return 1;
}

int is_while(ifjInter *self)
{
    token * active = lexa_next_token(self->lexa_module,self->table);
    if (active->type != T_WHILE)
    {
        print_unexpected(self, active);
        self->returnCode = 2;
        return 0;
    }

    return 1;
}

int is_LPAREN(ifjInter *self)
{
    token * active = lexa_next_token(self->lexa_module,self->table);
    if (active->type != T_LPAREN)
    {
        print_unexpected(self, active);
        self->returnCode = 2;
        return 0;
    }

    return 1;
}

int is_RPAREN(ifjInter *self)
{
    token * active = lexa_next_token(self->lexa_module,self->table);
    if (active->type != T_RPAREN)
    {
        print_unexpected(self, active);
        self->returnCode = 2;
        return 0;
    }

    return 1;
}

int is_ASSIGN(ifjInter *self)
{
    token * active = lexa_next_token(self->lexa_module,self->table);
    if (active->type != T_ASSIGN)
    {
        print_unexpected(self, active);
        self->returnCode = 2;
        return 0;
    }

    return 1;
}

/**
 * Fetch next token. Expect "else", if unexpected, perform token push bezparametricka
 * @param self global structure
 * @param table table for current function
 * @return 1 if else, 1 if pushBack
 **/
int if_else1(ifjInter *self, symbolTable *table)
{
    token * active = lexa_next_token(self->lexa_module, table);
    if (active->type == T_ELSE)
    {
        return is_LBLOCK(self) &&
               statement_inside1(self, table);
    }
    self->pushBack = active;
    return 1;
}

/**
 * Fetch next token, expects "{"
 * @param self global structure
 * @return 1 if successful
 **/
int is_LBLOCK(ifjInter *self)
{
    token * active = lexa_next_token(self->lexa_module,self->table);
    if (active->type != T_LBLOCK)
    {
        print_unexpected(self, active);
        self->returnCode = 2;
        return 0;
    }

    return 1;
}

/**
 * Inside statement (while, if, for ...)
 * - no variable declaration here!
 * - Expects "}", "while", "for", "if", "break", "continue", "return", "id"
 * @param self global structure
 * @param table symbol table for current function
 * @return 1 if successful
 **/
int statement_inside1(ifjInter *self, symbolTable *table)
{
    token * active = NULL;
    if(self->pushBack)
    {
        active = self->pushBack;
        self->pushBack = NULL;
    }
    else
    {
        active = lexa_next_token(self->lexa_module, table);
    }
    switch (active->type)
    {
        //END
        case T_RBLOCK:
          return 1;

        case T_WHILE:
            return condition(self, table) &&
                   statement_inside1(self, table) &&
                   statement_inside1(self, table);

      /*  case T_FOR:
        return is_LPAREN(self) &&
               tell_me_type_without_void(self) &&
               is_ID(self) &&
               is_ASSIGN(self) &&
               expresion(self) &&
               is_semicolon(self) &&
               condition(self) &&
               is_semicolon(self) &&
               is_ID(self) &&
               is_ASSIGN(self) &&
               expresion(self) &&
               is_RPAREN(self) &&
               statement_inside(self) &&
               statement_inside1(self);

        case T_DO:
        return statement_inside(self) &&
               is_while(self) &&
               is_LPAREN(self) &&
               condition(self) &&
               is_RPAREN(self) &&
               is_semicolon(self) &&
               statement_inside1(self);*/
    /*
        case T_BREAK:
            return is_semicolon(self) &&
                   statement_inside1(self, table);*/
/*
        case T_CONTINUE:
            return is_semicolon(self) &&
                   statement_inside1(self, table);*/
        //TODO generate jump + else jump
        case T_IF:
            return condition(self, table) &&
                   statement_inside1(self, table) &&
                   if_else1(self, table) &&
                   statement_inside1(self, table);
            
        //TODO generate jump
        case T_RETURN:
            return expresion(self, table) &&
                   statement_inside1(self, table);

        case T_IDENTIFIER:
        {
            int rc;
            rc = resolve_identifier(self, table, &active, 0);

            if(rc)
            {
                return fce(self, table, active) &&
                       statement_inside1(self, table);
            }

            return rc;
        }
    }

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * After identifier. Expects "(" - func or "=" for expresion
 * @param self global structure
 * @param table symbol table for current function_parameters
 * @param last token/identifier
 * @return 1 if successful
 **/
int fce(ifjInter *self, symbolTable *table, token *item)
{
    token * active = lexa_next_token(self->lexa_module,self->table);
    if (active->type == T_LPAREN)
    {
        return function_parameters(self, table, item);
    }
    else if (active->type == T_ASSIGN)
    {
        //TODO typing + instruction
        return expresion(self, table);
    }

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * Resolving functon call with return
 * fetch next token. Expect ")" or next function param
 * //TODO generate parameter push
 * @param self global structure
 * @param table symbol table for current function (not called one!)
 * @param item called function structure
 * @return 1 if successful
 **/
int function_parameters(ifjInter *self, symbolTable *table, token *item)
{
    token *active = lexa_next_token(self->lexa_module, table);
    token *expected = NULL;
    if(item->args && !ifj_stack_empty(item->args))
    {
        expected = item->args->elements[0];
    }
    if (active->type == T_IDENTIFIER)
    {
        int rc = resolve_identifier(self, table, &active, 0);
        if(!rc)
        {
            return rc;
        }
        if(expected && check_typing(active, expected))
        {
            if(1 > item->args->top)
            {
                expected = NULL;
            }
            else
            {
                expected = item->args->elements[1];
            }
            return next_function_parameters(self, table, item, expected, 1);
        }
        print_mistyped(self, active, expected);
        self->returnCode = 4;
        return 0;
    }
    else if(active->type == T_INTEGER_C ||
            active->type == T_DOUBLE_C ||
            active->type == T_STRING_C)
    {
        if(expected && check_typing(active, expected))
        {
            if(1 > item->args->top)
            {
                expected = NULL;
            }
            else
            {
                expected = item->args->elements[1];
            }
            return next_function_parameters(self, table, item, expected, 1);
        }
        print_mistyped(self, active, expected);
        self->returnCode = 4;
        return 0;
    }
    else if (active->type == T_RPAREN)
    {
        if(!item->args || (item->args && item->args->top < 0))
            return is_semicolon(self);
        print_mistyped(self, active, expected);
        self->returnCode = 4;
        return 0;
    }

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * Resolving function call, fetch next token. Expect ");" or next func param
 * //TODO generate parameter push
 * @param self global structure
 * @param table symbol table for current function (not called one!)
 * @param item called function structure
 * @return 1 if successful
 **/
int next_function_parameters(ifjInter *self,
                             symbolTable *table,
                             token *item,
                             token *expected,
                             int idx)
{
    token *active = lexa_next_token(self->lexa_module, self->table);
    if (active->type == T_RPAREN)
    {
        if(item->args && item->args->top == idx - 1 )
            return is_semicolon(self);
        print_mistyped(self, active, expected);
        self->returnCode = 4;
        return 0;

    }
    else if (active->type == T_COMMA)
    {
        int rc = next_param(self, table, expected);
        if(rc)
        {
            if(idx > item->args->top)
            {
                expected = NULL;
            }
            else
            {
                idx++;
                expected = item->args->elements[idx];
            }
            return next_function_parameters(self, table, item, expected, idx);
        }
        return rc;
    }
    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * Fetch next token. Expect "=" or ";".
 * @param self global structure
 * @param table symbol table for current function
 * @param item last token/identifier for instruction generation/type control
 * @return 1 if successful
 **/
int sth_next(ifjInter *self, symbolTable *table, token *item)
{
    token * active = lexa_next_token(self->lexa_module, table);
    if (active->type == T_ASSIGN)
    {
        return expresion(self, table);
        // TODO generate instruction
    }
    else if (active->type == T_SEMICOLON)
    {
        return 1;
    }

    print_unexpected(self, active);
    self->returnCode = 2;
    return 0;
}

/**
 * Run parser
 * Performs preLoad, when successful continues to full check
 * @param self global structure
 * @return 0 if successful, else error code
 **/
int syna_run(ifjInter *self)
{
    int return_value = next_class(self);
    if(return_value)
    {
        self->preLoad = 0;
        ifj_lexa_rewind_input(self->lexa_module);
        return_value = next_class(self);
    }

    if(self->debugMode)
    {
        fprintf(stderr, "Syna result: %d\n", self->returnCode);
    }

    if(self->debugMode)
        print_table(self->table, 0);

    return self->returnCode;
}

/**
 * Resolving functon call with return
 * fetch next token. Expect ")" or next function param
 * @param self global structure
 * @param table symbol table for current function (not called one!)
 * @param item called function structure
 * @return 1 if successful
 * TODO generate set
 **/
int function_parameters_for_exp(ifjInter *self,
                                symbolTable *table,
                                token *item)
{
    return function_parameters(self, table, item);
}
