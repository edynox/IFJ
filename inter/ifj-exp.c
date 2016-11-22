/* ifj-exp.c
*
* Copyright (C) 2016 SsYoloSwag41 Inc.
* Authors: Jan Demcak <xdemca01@stud.fit.vutbr.cz>
*/

/*TODO JANY Jiné než uvedené kombinace typů (včetně případných povolených implicitních kon-
verzí) ve výrazech pro popsané operátory jsou považovány za chybu 4. */

/*TODO JANY upavit tabulku priority operatorov != a == maju najnizsiu */


/*TODO JANY  vytvorit dalsi zasobnik ktory ked  dostanem konstantu alebo premennu pushnem ,
v momente ked sa bude vykonvat niejaka redukčná akcia ktora bude vyzadovat 2 operandy
skontroluje ak bude treba urobit typovie zavola Edovu funkciu na pretypovanie a
vysledny typ pushne na zasobnik s5 , nevyhodnocujem  vysledok  resp. nenazim sa manualne pocitat vysledok
to budu robit robove instrukcie ktorych volanie ja en d pisem do kodu ? TODO EDO skontroluj ci som dobre pochopil*/



/* TODO EDO , toto riesis ty  pri typovani ci ja ? Relační operátory nepodporují porovnání řetězců
(viz vestavěná funkce ifj16.compare ) */

#include "ifj-exp.h"

int condition(ifjInter *self, symbolTable *table)
{
    ifjSyna *syna = self->syna;

    int b;
    int a; // first symbol on stack is automatically $ --> 7;
    int rc;

    token * top_stack;
    token * top_on_help_stack;
    token * active = lexa_next_token(self->lexa_module, table);
    ifj_stack_clear(syna->stack);
    ifj_stack_clear(syna->help_stack);
    ifj_stack_clear(syna->type_stack);

    // if token is ID or constant  add token into  type_stack
   // type_stack is using for type control in expresion
   // there are 3 places where is this construction used
   // (always when function lexa_next_token() is called)
    if(active->type == T_IDENTIFIER ||
      active->type == T_STRING_C ||
      active->type == T_INTEGER_C ||
      active ->type == T_DOUBLE_C)
   {
       if(active->type == T_IDENTIFIER)
       {
           if ( (active->dataType == T_INTEGER_C) || (active->dataType == T_DOUBLE_C) )
           {
               ifj_stack_push(syna->type_stack, syna->t_integer);
           }
           else if ( active->dataType == T_STRING_C)
           {
               ifj_stack_push(syna->type_stack, syna->t_string);
           }
           else
           {
               return 0; self->returnCode = 4;
           }
       }
       else
       {
           ifj_stack_push(syna->type_stack, active);
       }
    }

    if(self->debugMode)
    {
    fprintf(stderr, "Som v condition\n");
    }

    ifj_stack_push(syna->stack, syna->lblock);
    top_stack = ifj_stack_top(syna->stack);

    do
    {
        switch (condition_check_active(active, &b))
        {
            case 0:
                return 0; // Nahradenie povodnej -1
                break;

            case 2:
            {
                rc = resolve_identifier(self, table, &active, 0);
                if(!rc)
                    return rc;
                break;
            }
        }


        if (!condition_check_top_stack(top_stack, &a))
            return 0; // nahradenie povodnej -1

        switch ((*syna->predictCondition)[a][b])
        {
            case T_EQUAL:
                ifj_stack_push(syna->stack, active);
                top_stack = active;
                active = lexa_next_token(self->lexa_module, table);
                if(active->type == T_IDENTIFIER ||
                  active->type == T_STRING_C ||
                  active->type == T_INTEGER_C ||
                  active ->type == T_DOUBLE_C)
               {
                   if(active->type == T_IDENTIFIER)
                   {
                       if ( (active->dataType == T_INTEGER_C) || (active->dataType == T_DOUBLE_C) )
                       {
                           ifj_stack_push(syna->type_stack, syna->t_integer);
                       }
                       else if ( active->dataType == T_STRING_C)
                       {
                           ifj_stack_push(syna->type_stack, syna->t_string);
                       }
                       else
                       {
                           return 0; self->returnCode = 4;
                       }
                   }
                   else
                   {
                       ifj_stack_push(syna->type_stack, active);
                   }
                }

                /*stale taha dalsie a dalsie tokeny bezime v do-while cykle*/
                break;

            case  T_LESS:
                if (ifj_stack_pop(syna->stack)->type == E_TYPE)
                {
                    top_stack = active; // new top of stack
                    // add T_LESS token on top of stack
                    ifj_stack_push(syna->stack, syna->t_less);
                    ifj_stack_push(syna->stack, syna->E);
                }
                else
                {
                    top_stack = active; // new top of stack
                    // add T_LESS token on top stack
                    ifj_stack_push(syna->stack, syna->t_less);
                }
                // add next token to the top of stack
                ifj_stack_push(syna->stack, top_stack);
                // take another token from scanner
                active = lexa_next_token(self->lexa_module, table);
                if(active->type == T_IDENTIFIER ||
                  active->type == T_STRING_C ||
                  active->type == T_INTEGER_C ||
                  active ->type == T_DOUBLE_C)
               {
                   if(active->type == T_IDENTIFIER)
                   {
                       if ( (active->dataType == T_INTEGER_C) || (active->dataType == T_DOUBLE_C) )
                       {
                           ifj_stack_push(syna->type_stack, syna->t_integer);
                       }
                       else if ( active->dataType == T_STRING_C)
                       {
                           ifj_stack_push(syna->type_stack, syna->t_string);
                       }
                       else
                       {
                           return 0; self->returnCode = 4;
                       }
                   }
                   else
                   {
                       ifj_stack_push(syna->type_stack, active);
                   }
                }

            break;

            case  T_END:
                return 0; // Nahradenie povodnej -1
                break;

            case T_GREATER:
                do // fullfiling the help_stack
                {
                    ifj_stack_push(syna->help_stack, ifj_stack_pop(syna->stack));
                } while(ifj_stack_top(syna->stack)->type != syna->t_less->type);

                ifj_stack_pop(syna->stack); // POP  T_LESS form stack
                top_on_help_stack = ifj_stack_pop(syna->help_stack);

                switch (top_on_help_stack->type)
                {
                    case T_LPAREN:
                        /* saving new top
                         * after T_LPAREN check
                         * if stack is empty else return 0 */
                        top_on_help_stack = ifj_stack_pop(syna->help_stack);
                        if (top_on_help_stack->type == E_TYPE) // E hash number
                        {
                            top_on_help_stack = ifj_stack_pop(syna->help_stack);
                            if (top_on_help_stack->type == T_RPAREN)
                            {
                                if (ifj_stack_empty(syna->help_stack))
                                {
                                    top_stack = ifj_stack_top(syna->stack);
                                     // push E on the top of main stack
                                    ifj_stack_push(syna->stack, syna->E);
                                    if(self->debugMode)
                                    {
                                        fprintf(stderr, "E --> (E)\n");
                                    }
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                            else
                            {
                                return 0;
                            }
                        }
                        else
                        {
                            return 0;
                        }
                    break;

                case T_IDENTIFIER:
                    if (ifj_stack_empty(syna->help_stack))
                    {

                        top_stack = ifj_stack_top(syna->stack);
                        ifj_stack_push(syna->stack, syna->E);
                        //TODO ROBO  v premennej top_on_help_stack je id instrukcia PUSH

                        if(self->debugMode)
                        {
                            fprintf(stderr, "E --> id\n");
                        }
                    }
                    else
                    {
                        return 0;
                    }
                    break;

                case T_STRING_C:
                    if (ifj_stack_empty(syna->help_stack))
                    {
                        top_stack = ifj_stack_top(syna->stack);
                        ifj_stack_push(syna->stack, syna->E);
                        //TODO ROBO  v premennej top_on_help_stack je string_c instrukcia PUSH
                        if(self->debugMode)
                        {
                            fprintf(stderr, "E --> string\n");
                        }
                    }
                    else
                    {
                        return 0;
                    }
                    break;

                case T_INTEGER_C:
                    if (ifj_stack_empty(syna->help_stack))
                    {
                        top_stack = ifj_stack_top(syna->stack);
                        ifj_stack_push(syna->stack, syna->E);
                        //TODO ROBO  v premennej top_on_help_stack je integer_c instrukcia PUSH
                        if(self->debugMode)
                        {
                            fprintf(stderr, "E --> integer\n");
                        }
                    }
                    else
                    {
                        return 0;
                    }
                    break;

                case T_DOUBLE_C:
                    if (ifj_stack_empty(syna->help_stack))
                    {
                        top_stack = ifj_stack_top(syna->stack);
                        ifj_stack_push(syna->stack, syna->E);
                        //TODO ROBO  v premennej top_on_help_stack je double_c instrukcia PUSH
                        if(self->debugMode)
                        {
                            fprintf(stderr, "E --> double\n");
                        }
                    }
                    else
                    {
                        return 0;
                    }
                    break;

                case E_TYPE: //E value
                    top_on_help_stack = ifj_stack_pop(syna->help_stack);

                    switch (top_on_help_stack->type)
                    {
                        case T_ADD:
                            top_on_help_stack = ifj_stack_pop(syna->help_stack);
                            if (top_on_help_stack == syna->E)
                            {
                                if(ifj_stack_empty(syna->help_stack))
                                {
                                    top_stack = ifj_stack_top(syna->stack);
                                    ifj_stack_push(syna->stack, syna->E);
                                    if(type_control_plus(self) == 4)
                                    {
                                      self->returnCode = 4;
                                      return 0;
                                    }
                                    //TODO ROBO instrukcia ADD
                                    if(self->debugMode)
                                    {
                                        fprintf(stderr, "E --> E + E\n");
                                    }
                                    // TODO ROBO teoreticka instrukcia
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                            else
                            {
                                return 0;
                            }
                            break;

                        case T_SUBTRACT:
                            top_on_help_stack = ifj_stack_pop(syna->help_stack);
                            if (top_on_help_stack == syna->E)
                            {
                                if(ifj_stack_empty(syna->help_stack))
                                {
                                    top_stack = ifj_stack_top(syna->stack);
                                    ifj_stack_push(syna->stack, syna->E);
                                    if(type_control(self) == 4)
                                    {
                                      return 0; self->returnCode = 4;
                                    }
                                    //TODO ROBO instrukcia SUB
                                    if(self->debugMode)
                                    {
                                        fprintf(stderr, "E --> E - E\n");
                                    }
                                    // TODO ROBO teoreticka instrukcia
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                            else
                            {
                                return 0;
                            }
                            break;

                        case T_DIVIDE:
                            top_on_help_stack = ifj_stack_pop(syna->help_stack);
                            if (top_on_help_stack == syna->E)
                            {
                                if(ifj_stack_empty(syna->help_stack))
                                {
                                    top_stack = ifj_stack_top(syna->stack);
                                    ifj_stack_push(syna->stack, syna->E);
                                    if(type_control(self) == 4)
                                    {
                                      return 0; self->returnCode = 4;
                                    }
                                    //TODO ROBO instrukcia DIV
                                    if(self->debugMode)
                                    {
                                        fprintf(stderr, "E --> E / E\n");

                                    }
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                            else
                            {
                                return 0;
                            }
                            break;

                        case T_MULTIPLY:
                            top_on_help_stack = ifj_stack_pop(syna->help_stack);
                            if (top_on_help_stack == syna->E)
                            {
                                if(ifj_stack_empty(syna->help_stack))
                                {
                                    top_stack = ifj_stack_top(syna->stack);
                                    ifj_stack_push(syna->stack, syna->E);
                                    if(type_control(self) == 4)
                                    {
                                      return 0; self->returnCode = 4;
                                    }
                                    //TODO ROBO instrukcia MUL
                                    if(self->debugMode)
                                    {
                                        fprintf(stderr, "E --> E * E\n");

                                    }
                                    // TODO ROBO teoreticka instrukcia
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                            else
                            {
                                return 0;
                            }
                            break;

                        case T_GREATER:
                            top_on_help_stack = ifj_stack_pop(syna->help_stack);
                            if (top_on_help_stack == syna->E)
                            {
                                if (ifj_stack_empty(syna->help_stack))
                                {
                                    top_stack = ifj_stack_top(syna->stack);
                                    ifj_stack_push(syna->stack, syna->E);
                                    if(type_control(self) == 4)
                                    {
                                      return 0; self->returnCode = 4;
                                    }
                                    //TODO ROBO instrukcia condition parameter >
                                    if(self->debugMode)
                                    {
                                        fprintf(stderr, "E --> E > E\n");

                                    }
                                    // TODO ROBO teoreticka instrukcia
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                            else
                            {
                                return 0;
                            }
                            break;

                        case T_LESS:
                            top_on_help_stack = ifj_stack_pop(syna->help_stack);
                            if (top_on_help_stack == syna->E)
                            {
                                if (ifj_stack_empty(syna->help_stack))
                                {
                                    top_stack = ifj_stack_top(syna->stack);
                                    ifj_stack_push(syna->stack, syna->E);
                                    if(type_control(self) == 4)
                                    {
                                      return 0; self->returnCode = 4;
                                    }
                                    //TODO ROBO instrukcia condition parameter <
                                    if(self->debugMode)
                                    {
                                        fprintf(stderr, "E --> E < E\n");
                                    }
                                    // TODO ROBO teoreticka instrukcia
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                            else
                            {
                                return 0;
                            }
                            break;

                        case T_EQUAL:
                            top_on_help_stack = ifj_stack_pop(syna->help_stack);
                            if (top_on_help_stack == syna->E)
                            {
                                if (ifj_stack_empty(syna->help_stack))
                                {
                                    top_stack = ifj_stack_top(syna->stack);
                                    ifj_stack_push(syna->stack, syna->E);
                                    if(type_control(self) == 4)
                                    {
                                      return 0; self->returnCode = 4;
                                    }
                                    //TODO ROBO instrukcia condition parameter ==
                                    if(self->debugMode)
                                    {
                                        fprintf(stderr, "E --> E == E\n");
                                    }
                                    // TODO ROBO teoreticka instrukcia
                                }
                                else
                                {
                                    return 0;
                                }

                            }
                            else
                            {
                                return 0;
                            }
                            break;

                        case T_GREATER_EQUAL:
                            top_on_help_stack = ifj_stack_pop(syna->help_stack);
                            if (top_on_help_stack == syna->E)
                            {
                                if (ifj_stack_empty(syna->help_stack))
                                {
                                    top_stack = ifj_stack_top(syna->stack);
                                    ifj_stack_push(syna->stack, syna->E);
                                    if(type_control(self) == 4)
                                    {
                                      return 0; self->returnCode = 4;
                                    }
                                    //TODO ROBO instrukcia condition parameter >=
                                    if(self->debugMode)
                                    {
                                        fprintf(stderr, "E --> E >= E\n");
                                    }
                                    // TODO ROBO teoreticka instrukcia
                                }
                                else
                                {
                                    return 0;
                                }

                            }
                            else
                            {
                                return 0;
                            }
                            break;

                        case T_LESS_EQUAL:
                            top_on_help_stack = ifj_stack_pop(syna->help_stack);
                            if (top_on_help_stack == syna->E)
                            {
                                if (ifj_stack_empty(syna->help_stack))
                                {
                                    top_stack = ifj_stack_top(syna->stack);
                                    ifj_stack_push(syna->stack, syna->E);
                                    if(type_control(self) == 4)
                                    {
                                      return 0; self->returnCode = 4;
                                    }
                                    //TODO ROBO instrukcia condition parameter <=
                                    if(self->debugMode)
                                    {
                                        fprintf(stderr, "E --> E <= E\n");
                                    }
                                    // TODO ROBO teoreticka instrukcia
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                            else
                            {
                                return 0;
                            }
                            break;

                        case T_NOT_EQUAL:
                            top_on_help_stack = ifj_stack_pop(syna->help_stack);
                            if (top_on_help_stack == syna->E)
                            {
                                if (ifj_stack_empty(syna->help_stack))
                                {
                                    top_stack = ifj_stack_top(syna->stack);
                                    ifj_stack_push(syna->stack, syna->E);
                                    if(type_control(self) == 4)
                                    {
                                      return 0; self->returnCode = 4;
                                    }
                                    //TODO ROBO instrukcia condition parameter !=
                                    if(self->debugMode)
                                    {
                                        fprintf(stderr, "E --> E != E\n");
                                    }
                                    // TODO ROBO teoreticka instrukcia
                                }
                                else
                                {
                                    return 0;
                                }

                            }
                            else
                            {
                                return 0;
                            }
                            break;

                        default:
                        return 0;
                        break;
                    }
            }
            break;
        }
    } while((active->type != T_LBLOCK) || (top_stack->type != T_LBLOCK));

    if(self->debugMode)
    {
        fprintf(stderr, "vraciam sa z condition\n");
    }
    return 1;
}

int expresion(ifjInter *self, symbolTable *table)
{
    if(self->debugMode)
    {
        fprintf(stderr, "som v expresion\n");
    }
    ifjSyna *syna = self->syna;

    int b;
    int a; // first symbol on stack is automatically $ --> 7;
    int rc;

    token * top_stack;
    token * top_on_help_stack;
    token * active = lexa_next_token(self->lexa_module, table);
    ifj_stack_clear(syna->stack);
    ifj_stack_clear(syna->help_stack);
    ifj_stack_clear(syna->type_stack);

    if(active->type == T_IDENTIFIER ||
      active->type == T_STRING_C ||
      active->type == T_INTEGER_C ||
      active ->type == T_DOUBLE_C)
   {
       if(active->type == T_IDENTIFIER)
       {
           if ( (active->dataType == T_INTEGER_C) || (active->dataType == T_DOUBLE_C) )
           {
               ifj_stack_push(syna->type_stack, syna->t_integer);
           }
           else if ( active->dataType == T_STRING_C)
           {
               ifj_stack_push(syna->type_stack, syna->t_string);
           }
           else
           {
               return 0; self->returnCode = 4;
           }
       }
       else
       {
           ifj_stack_push(syna->type_stack, active);
       }
    }

    ifj_stack_push(syna->stack, syna->semicolon);
    top_stack = ifj_stack_top(syna->stack);

    do
    {
        switch (expresion_check_active(active, &b))
        {
            case 0:
                return 0; // Nahradenie povodnej -1
                break;

            case 2:
                rc = resolve_identifier(self, table, &active, 0);
                if (!rc)
                    return 0;
                break;
        }


        if (!expresion_check_top_stack(top_stack, &a))
            return 0; // nahradenie povodnej -1



      /*  if(self->debugMode)
        {
            fprintf(stderr, "a je %d a b je %d\n",a,b);
        }*/
        switch ((*syna->predictExpresion)[a][b])
        {
            case T_EQUAL:
                ifj_stack_push(syna->stack,active);
                top_stack = ifj_stack_top(syna->stack);
                active = lexa_next_token(self->lexa_module, table);
                if(active->type == T_IDENTIFIER ||
                  active->type == T_STRING_C ||
                  active->type == T_INTEGER_C ||
                  active ->type == T_DOUBLE_C)
               {
                   if(active->type == T_IDENTIFIER)
                   {
                       if ( (active->dataType == T_INTEGER_C) || (active->dataType == T_DOUBLE_C) )
                       {
                           ifj_stack_push(syna->type_stack, syna->t_integer);
                       }
                       else if ( active->dataType == T_STRING_C)
                       {
                           ifj_stack_push(syna->type_stack, syna->t_string);
                       }
                       else
                       {
                           return 0; self->returnCode = 4;
                       }
                   }
                   else
                   {
                       ifj_stack_push(syna->type_stack, active);
                   }
                }

                break;

            case  T_LESS:
                if (ifj_stack_top(syna->stack)->type == E_TYPE)
                {
                    ifj_stack_pop(syna->stack);
                    ifj_stack_push(syna->stack, syna->t_less); // add T_LESS token on top stack
                    ifj_stack_push(syna->stack, syna->E);
                    top_stack = active; // new top of stack
                }
                else
                {
                    ifj_stack_push(syna->stack, syna->t_less); // add T_LESS token on top stack
                    top_stack = active; // new top of stack
                }

                ifj_stack_push(syna->stack, top_stack); // add next token to the top of stack
                active = lexa_next_token(self->lexa_module, table); // take another token from scanner
                if(active->type == T_IDENTIFIER ||
                  active->type == T_STRING_C ||
                  active->type == T_INTEGER_C ||
                  active ->type == T_DOUBLE_C)
               {
                   if(active->type == T_IDENTIFIER)
                   {
                       if ( (active->dataType == T_INTEGER_C) || (active->dataType == T_DOUBLE_C) )
                       {
                           ifj_stack_push(syna->type_stack, syna->t_integer);
                       }
                       else if ( active->dataType == T_STRING_C)
                       {
                           ifj_stack_push(syna->type_stack, syna->t_string);
                       }
                       else
                       {
                           return 0; self->returnCode = 4;
                       }
                   }
                   else
                   {
                       ifj_stack_push(syna->type_stack, active);
                   }
                }

                break;
            /* musim niekde ulozit token co je na vrchu zasobniku,
            to je ID funkcie a predat ho rekurzivnemu sestupu
            + nezabudnut pridat "(" */
            case  T_COMMA:
                return function_parameters_for_exp(self, table, ifj_stack_top(syna->stack));

            case  T_END:
                return 0;

            case T_GREATER:
                do // will fulling help_stack which one will using next
                {
                    ifj_stack_push(syna->help_stack, ifj_stack_pop(syna->stack));
                } while(ifj_stack_top(syna->stack)->type != syna->t_less->type);

                ifj_stack_pop(syna->stack); // POP  T_LESS form stack
                top_on_help_stack = ifj_stack_pop(syna->help_stack);

                switch (top_on_help_stack->type)
                {
                    case T_LPAREN:
                        top_on_help_stack = ifj_stack_pop(syna->help_stack);
                        if (top_on_help_stack->type == E_TYPE) // E hash number
                        {
                            top_on_help_stack = ifj_stack_pop(syna->help_stack);
                            if (top_on_help_stack->type == T_RPAREN)
                            {
                                if (ifj_stack_empty(syna->help_stack))
                                {
                                    top_stack = ifj_stack_top(syna->stack);
                                    ifj_stack_push(syna->stack, syna->E); // push E on the top of main stack

                                    if(self->debugMode)
                                    {
                                        fprintf(stderr, "E --> (E)\n");

                                    }
                                }
                                else
                                {
                                    return 0;
                                }
                            }
                            else
                            {
                                return 0;
                            }
                        }
                        else
                        {
                            return 0;
                        }
                        break;

                    case T_IDENTIFIER:
                        if (ifj_stack_empty(syna->help_stack))
                        {
                            top_stack = ifj_stack_top(syna->stack);
                            ifj_stack_push(syna->stack, syna->E);
                            //TODO ROBO  v premennej top_on_help_stack je id instrukcia PUSH
                            if(self->debugMode)
                            {
                                fprintf(stderr, "E --> id\n");
                            }
                        }
                        else
                        {
                            return 0;
                        }
                        break;

                    case T_STRING_C:
                        if (ifj_stack_empty(syna->help_stack))
                        {
                            top_stack = ifj_stack_top(syna->stack);
                            ifj_stack_push(syna->stack, syna->E);
                            //TODO ROBO  v premennej top_on_help_stack je string_c instrukcia PUSH
                            if(self->debugMode)
                            {
                                fprintf(stderr, "E --> string\n");
                            }
                        }
                        else
                        {
                            return 0;
                        }
                        break;

                    case T_INTEGER_C:
                        if (ifj_stack_empty(syna->help_stack))
                        {
                            top_stack = ifj_stack_top(syna->stack);
                            ifj_stack_push(syna->stack, syna->E);
                            //TODO ROBO  v premennej top_on_help_stack je integer_c instrukcia PUSH
                            if(self->debugMode)
                            {
                                fprintf(stderr, "E --> integer\n");
                            }
                        }
                        else
                        {
                            return 0;
                        }
                        break;

                    case T_DOUBLE_C:
                    ifj_stack_pop(syna->help_stack);
                        if (ifj_stack_empty(syna->help_stack))
                        {
                            top_stack = ifj_stack_top(syna->stack);
                            ifj_stack_push(syna->stack, syna->E);
                            //TODO ROBO  v premennej top_on_help_stack je double_c instrukcia PUSH
                            if(self->debugMode)
                            {
                                fprintf(stderr, "E --> double\n");
                            }
                        }
                        else
                        {
                            return 0;
                        }
                        break;

                    case E_TYPE: //E value
                        top_on_help_stack = ifj_stack_pop(syna->help_stack);
                        switch (top_on_help_stack->type)
                        {
                            case T_ADD:
                                top_on_help_stack = ifj_stack_pop(syna->help_stack);
                                if (top_on_help_stack == syna->E)
                                {
                                    if(ifj_stack_empty(syna->help_stack))
                                    {
                                        top_stack = ifj_stack_top(syna->stack);
                                        ifj_stack_push(syna->stack, syna->E);
                                        if(type_control_plus(self) == 4)
                                        {
                                          self->returnCode = 4;
                                          return 0;
                                        }
                                        //TODO ROBO instrukcia ADD
                                        if(self->debugMode)
                                        {
                                            fprintf(stderr, "E --> E + E\n");
                                        }
                                    }
                                    else
                                    {
                                        return 0;
                                    }
                                }
                                else
                                {
                                    return 0;
                                }
                                break;

                            case T_SUBTRACT:
                                top_on_help_stack = ifj_stack_pop(syna->help_stack);
                                if (top_on_help_stack == syna->E)
                                {
                                    if(ifj_stack_empty(syna->help_stack))
                                    {
                                        top_stack = ifj_stack_top(syna->stack);
                                        ifj_stack_push(syna->stack, syna->E);
                                        if(type_control(self) == 4)
                                        {
                                          return 0; self->returnCode = 4;
                                        }
                                        //TODO ROBO instrukcia SUB
                                        if(self->debugMode)
                                        {
                                            fprintf(stderr, "E --> E - E\n");
                                        }
                                    }
                                    else
                                    {
                                        return 0;
                                    }
                                }
                                else
                                {
                                    return 0;
                                }
                                break;

                            case T_DIVIDE:
                                top_on_help_stack = ifj_stack_pop(syna->help_stack);
                                if (top_on_help_stack == syna->E)
                                {
                                    if(ifj_stack_empty(syna->help_stack))
                                    {
                                        top_stack = ifj_stack_top(syna->stack);
                                        ifj_stack_push(syna->stack, syna->E);
                                        if(type_control(self) == 4)
                                        {
                                          return 0; self->returnCode = 4;
                                        }
                                        //TODO ROBO instrukcia DIV
                                        if(self->debugMode)
                                        {
                                            fprintf(stderr, "E --> E / E\n");

                                        }
                                    }
                                    else
                                    {
                                        return 0;
                                    }
                                }
                                else
                                {
                                    return 0;
                                }
                                break;

                            case T_MULTIPLY:
                                top_on_help_stack = ifj_stack_pop(syna->help_stack);
                                if (top_on_help_stack == syna->E)
                                {
                                    if(ifj_stack_empty(syna->help_stack))
                                    {
                                        top_stack = ifj_stack_top(syna->stack);
                                        ifj_stack_push(syna->stack, syna->E);
                                        if(type_control(self) == 4)
                                        {
                                          return 0; self->returnCode = 4;
                                        }
                                        //TODO ROBO instrukcia MUL
                                        if(self->debugMode)
                                        {
                                            fprintf(stderr, "E --> E * E\n");
                                        }
                                    }
                                    else
                                    {
                                        return 0;
                                    }
                                }
                                else
                                {
                                    return 0;
                                }
                                break;
                            default:
                                return 0;
                        }
                        break;
                    default:
                        return 0;
            }
        }

    } while((active->type != T_SEMICOLON) || (top_stack->type != T_SEMICOLON));

    if(self->debugMode)
    {
        fprintf(stderr, "vraciam sa z expresion\n");
    }
    return 1;
}

inline int condition_check_active(token *active, int *b)
{
    switch (active->type)
    {
        case T_ADD:
            *b = 0;
            return 1;

        case T_SUBTRACT:
            *b = 1;
            return 1;

        case T_MULTIPLY:
            *b = 2;
            return 1;

        case T_DIVIDE:
            *b = 3;
            return 1;

        case T_LPAREN:
            *b = 4;
            return 1;

        case T_RPAREN:
            *b = 5;
            return 1;

        case T_IDENTIFIER:
            *b = 6;
            // return_value = resolve_identifier(self, table, &active, 0);
            return 2;

        case T_LBLOCK:
            *b = 7;
            return 1;

        case T_STRING_C:
            *b = 8;
            return 1;

        case T_INTEGER_C:
            *b = 9;
            return 1;

        case T_DOUBLE_C:
            *b = 10;
            return 1;

        case T_GREATER:
            *b = 11;
            return 1;

        case T_LESS:
            *b = 12;
            return 1;

        case T_EQUAL:
            *b = 13;
            return 1;

        case T_GREATER_EQUAL:
            *b = 14;
            return 1;

        case T_LESS_EQUAL:
            *b = 15;
            return 1;

        case T_NOT_EQUAL:
            *b = 16;
            return 1;
    }
    *b = -1;
    return 0;
}

inline int condition_check_top_stack(token *top_stack, int *a)
{
    switch (top_stack->type)
    {
        case T_ADD:
            *a = 0;
            return 1;

        case T_SUBTRACT:
            *a = 1;
            return 1;

        case T_MULTIPLY:
            *a = 2;
            return 1;

        case T_DIVIDE:
            *a = 3;
            return 1;

        case T_LPAREN:
            *a = 4;
            return 1;

        case T_RPAREN:
            *a = 5;
            return 1;

        case T_IDENTIFIER:
            *a = 6;
            return 1;

        case T_LBLOCK:
            *a = 7;
            return 1;

        case T_STRING_C:
            *a = 8;
            return 1;

        case T_INTEGER_C:
            *a = 9;
            return 1;

        case T_DOUBLE_C:
            *a = 10;
            return 1;

        case T_GREATER:
            *a = 11;
            return 1;

        case T_LESS:
            *a = 12;
            return 1;

        case T_EQUAL:
            *a = 13;
            return 1;

        case T_GREATER_EQUAL:
            *a = 14;
            return 1;

        case T_LESS_EQUAL:
            *a = 15;
            return 1;

        case T_NOT_EQUAL:
            *a = 16;
            return 1;

    }
    *a = -1;
    return 0;
}

extern inline int expresion_check_active(token *active, int *b)
{
    switch (active->type)
    {
        case T_ADD:
            *b = 0;
            return 1;

        case T_SUBTRACT:
            *b = 1;
            return 1;

        case T_MULTIPLY:
            *b = 2;
            return 1;

        case T_DIVIDE:
            *b = 3;
            return 1;

        case T_LPAREN:
            *b = 4;
            return 1;

        case T_RPAREN:
            *b = 5;
            return 1;

        case T_IDENTIFIER:
            //resolve_identifier(self, table, &active, 0);
            *b = 6;
            return 2;

        case T_SEMICOLON:
            *b = 7;
            return 1;

        case T_STRING_C:
            *b = 8;
            return 1;

        case T_INTEGER_C:
            *b = 9;
            return 1;

        case T_DOUBLE_C:
            *b = 10;
            return 1;
    }

    *b = -1;
    return 0;
}
extern inline int expresion_check_top_stack(token *top_stack, int *a)
{
    switch (top_stack->type)
    {
        case T_ADD:
            *a = 0;
            return 1;

        case T_SUBTRACT:
            *a = 1;
            return 1;

        case T_MULTIPLY:
            *a = 2;
            return 1;

        case T_DIVIDE:
            *a = 3;
            return 1;

        case T_LPAREN:
            *a = 4;
            return 1;

        case T_RPAREN:
            *a = 5;
            return 1;

        case T_IDENTIFIER:
            *a = 6;
            return 1;

        case T_SEMICOLON:
            *a = 7;
            return 1;

        case T_STRING_C:
            *a = 8;
            return 1;

        case T_INTEGER_C:
            *a = 9;
            return 1;

        case T_DOUBLE_C:
            *a = 10;
            return 1;
    }
    *a = -1;
    return 0;
}

int type_control(ifjInter *self)
{
  ifjSyna *syna = self->syna;
  token * first_type_stack_token = ifj_stack_pop(syna->type_stack);
  token * second_type_stack_token = ifj_stack_pop(syna->type_stack);
  if ((first_type_stack_token->type == T_STRING_C) || (second_type_stack_token->type == T_STRING_C))
  {
      return 4;
  }
  else
  {
      ifj_stack_push(syna->type_stack, syna->t_integer);
      return 0;
  }
}

int type_control_plus(ifjInter *self)
{
  ifjSyna *syna = self->syna;
  token * first_type_stack_token = ifj_stack_pop(syna->type_stack);
  token * second_type_stack_token = ifj_stack_pop(syna->type_stack);
  if ((first_type_stack_token->type == T_STRING_C) && (second_type_stack_token->type == T_STRING_C))
  {
      ifj_stack_push(syna->type_stack, syna->t_string);
      return 0;

  }
  else if ((first_type_stack_token->type == T_STRING_C) || (second_type_stack_token->type == T_STRING_C))
  {
      return 4;
  }
  else
  {
      ifj_stack_push(syna->type_stack, syna->t_integer);
      return 0;
  }
}
