/* ifj-lexa.c
 *
 * Copyright (C) 2016 SsYoloSwag41 Inc.
 * Authors: Jakub Kulich <xkulic03@stud.fit.vutbr.cz>
 */

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "ifj_lexa.h"
#include "limits.h"

ifj_lexa *ifj_lexa_init(ifjInter *inter) {
    ifj_lexa *l = calloc(1,sizeof(ifj_lexa));
    if (l == NULL) {
        return NULL;
    }

    l->inputFile = NULL;
    l->inter = inter;

    // 29 for minimalising colision in hash table
    l->reserved_words = ial_symbol_table_new(41);
    l->b_str = dyn_buffer_init(64);
    l->b_num = dyn_buffer_init(16);
    l->line_number = 1;

    token *item;
    ifj_generate_reserved(l->reserved_words, "while", T_WHILE, 0);
    ifj_generate_reserved(l->reserved_words, "for", T_FOR, 0);
    ifj_generate_reserved(l->reserved_words, "do", T_DO, 0);
    ifj_generate_reserved(l->reserved_words, "break", T_BREAK, 0);
    ifj_generate_reserved(l->reserved_words, "continue", T_CONTINUE, 0);
    ifj_generate_reserved(l->reserved_words, "if", T_IF, 0);
    ifj_generate_reserved(l->reserved_words, "else", T_ELSE, 0);
    ifj_generate_reserved(l->reserved_words, "return", T_RETURN, 0);
    ifj_generate_reserved(l->reserved_words, "void", T_VOID, 0);
    ifj_generate_reserved(l->reserved_words, "static", T_STATIC, 0);
    ifj_generate_reserved(l->reserved_words, "class", T_CLASS, 0);
    ifj_generate_reserved(l->reserved_words, "boolean", T_BOOLEAN, 0);
    item = ifj_generate_reserved(l->reserved_words, "int", T_INTEGER, 0);
    item->dataType = T_INTEGER;
    item = ifj_generate_reserved(l->reserved_words, "double", T_DOUBLE, 0);
    item->dataType = T_DOUBLE;
    item = ifj_generate_reserved(l->reserved_words, "String", T_STRING, 0);
    item->dataType = T_STRING;
    ifj_generate_reserved(l->reserved_words, "false", T_FALSE, 0);
    ifj_generate_reserved(l->reserved_words, "true", T_TRUE, 0);

    return l;
}

void ifj_lexa_free(ifj_lexa *l) {
    if(l->inputFile)
        fclose(l->inputFile);
    ial_symbol_table_drop(l->reserved_words);
    dyn_buffer_free(l->b_str);
    dyn_buffer_free(l->b_num);
    free(l);
}

void ifj_lexa_rewind_input(ifj_lexa *lexa) {
    lexa->line_number = 1;
    rewind(lexa->inputFile);
}

int ifj_lexa_is_reserved(ifj_lexa *l, char *word) {
    token *item = ial_symbol_table_get_item(l->reserved_words, word, 0, NULL);
    if (item == NULL) {
        return -1;
    } else {
        return item->type;
    }
}

static int ishexadigit(int character) {
    if (isdigit(character) || (character >= 65 && character <= 70)
        || (character >= 97 && character <= 102)) {
        return 1;
    } else {
        return 0;
    }
}

token *lexa_next_token(ifj_lexa *l, symbolTable *table) {

    int newChar = 0;

    enum lexa_state state;
    state = LS_START;

    dyn_buffer_clear(l->b_str);
    dyn_buffer_clear(l->b_num);

    token *t = NULL;

    while (1) {
        newChar = getc(l->inputFile);

        switch (state) {
            case LS_START:
                if (newChar == '0') {
                    dyn_buffer_append(l->b_str, newChar);
                    state = LS_NUMBER_ZERO;
                    break;
                } else if (isdigit(newChar)) {
                    dyn_buffer_append(l->b_str, newChar);
                    state = LS_NUMBER;
                    break;
                } else if (isalpha(newChar)) {
                    dyn_buffer_append(l->b_str, newChar);
                    state = LS_WORD;
                    break;
                } else if (isspace(newChar)) {
                    if (newChar == '\n') l->line_number++;
                    break;
                } else if (newChar == EOF) {
                    t = ifj_generate_token(table, T_END);
                    return t;
                } else if (newChar == '/') {
                    state = LS_DIV;
                    break;
                } else if (newChar == '\"') {
                    state = LS_STRING;
                    break;
                } else {
                    switch (newChar) {
                        case '_':
                        case '$':
                            dyn_buffer_append(l->b_str, newChar);
                            state = LS_WORD;
                            break;
                        case '<':
                            state = LS_COMPARE_LESS;
                            break;
                        case '>':
                            state = LS_COMPARE_GREATER;
                            break;
                        case '=':
                            state = LS_EQUAL;
                            break;
                        case '!':
                            state = LS_NEQ;
                            break;
                        case '+':
                        case '-':
                        case ';':
                        case '(':
                        case ')':
                        case '{':
                        case '}':
                        case '[':
                        case ']':
                        case '*':
                        case ',':
                            t = ifj_generate_token(table, newChar);
                            return t;
                        default:
                            l->inter->returnCode = 1;
                            t = ifj_generate_token(table, T_UNKNOWN);
                            return t;

                    }
                }

                break;
            case LS_DIV:
                if (newChar == '/') {
                    state = LS_COMMENT;
                    break;
                } else if (newChar == '*') {
                    state = LS_MULTI_COMMENT;
                    break;
                } else {
                    ungetc(newChar, l->inputFile);
                    t = ifj_generate_token(table, T_DIVIDE);
                    return t;
                }
                break;
            case LS_COMMENT:
                if (newChar == '\n') {
                    l->line_number++;
                    state = LS_START;
                    break;
                } else if (newChar == EOF) {
                    t = ifj_generate_token(table, T_END);
                    return t;
                }
                break;
            case LS_MULTI_COMMENT:
                if (newChar == '*') {
                    state = LS_MULTI_COMMENT_END;
                    break;
                } else if (newChar == '\n') {
                    l->line_number++;
                } else if (newChar == EOF) {
                    l->inter->returnCode = 1;
                    t = ifj_generate_token(table, T_UNKNOWN);
                    return t;
                }
                break;
            case LS_MULTI_COMMENT_END:
                if (newChar == '/') {
                    state = LS_START;
                    break;
                } else if (newChar == '*') {
                    break;
                } else if (newChar == EOF) {
                    l->inter->returnCode = 1;
                    t = ifj_generate_token(table, T_UNKNOWN);
                    return t;
                } else {
                    state = LS_MULTI_COMMENT;
                    break;
                }
                break;
            case LS_STRING:
                if (newChar == '\"') {
                    t = ifj_generate_token_str(table, dyn_buffer_get_content(l->b_str));
                    return t;
                } else if (newChar == '\\') {
                    state = LS_ESCAPE;
                    break;
                } else if (newChar == '\n') {
                    l->inter->returnCode = 1;
                    t = ifj_generate_token(table, T_UNKNOWN);
                    return t;
                } else if (newChar >= 32 && newChar <= 255) {
                    dyn_buffer_append(l->b_str, newChar);
                } else {
                    l->inter->returnCode = 1;
                    t = ifj_generate_token(table, T_UNKNOWN);
                    return t;
                }
                break;
            case LS_ESCAPE:
                if (isdigit(newChar)) {
                    state = LS_ESCAPE_OCTAL;
                    dyn_buffer_append(l->b_num, newChar);
                    break;
                } else if (newChar == EOF) {
                    l->inter->returnCode = 1;
                    t = ifj_generate_token(table, T_UNKNOWN);
                    return t;
                } else {
                    state = LS_STRING;

                    switch (newChar) {
                        case '\"':
                            dyn_buffer_append(l->b_str, '\"');
                            break;
                        case '\\':
                            dyn_buffer_append(l->b_str, '\\');
                            break;
                        case 'n':
                            dyn_buffer_append(l->b_str, '\n');
                            break;
                        case 't':
                            dyn_buffer_append(l->b_str, '\t');
                            break;
                        default:
                            l->inter->returnCode = 1;
                            t = ifj_generate_token(table, T_UNKNOWN);
                            return t;
                    }
                }
                break;
            case LS_ESCAPE_OCTAL:
                if (l->b_num->top == 2) {
                    char *endptr = NULL;
                    errno = 0;
                    int escChar = (int) strtol(dyn_buffer_get_content(l->b_num), &endptr, 8);
                    if (*endptr != '\0' || errno != 0 ||
                            escChar < 1 || escChar > 255) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }

                    dyn_buffer_clear(l->b_num);
                    dyn_buffer_append(l->b_str, escChar);

                    ungetc(newChar, l->inputFile);
                    state = LS_STRING;
                } else {
                    if (isdigit(newChar)) {
                        dyn_buffer_append(l->b_num, newChar);
                    } else {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                }
                break;
            case LS_COMPARE_GREATER:
                if (newChar == '=') {
                    t = ifj_generate_token(table, T_GREATER_EQUAL);
                    return t;
                } else {
                    ungetc(newChar, l->inputFile);
                    t = ifj_generate_token(table, T_GREATER);
                    return t;
                }
            case LS_COMPARE_LESS:
                if (newChar == '=') {
                    t = ifj_generate_token(table, T_LESS_EQUAL);
                    return t;
                } else {
                    ungetc(newChar, l->inputFile);
                    t = ifj_generate_token(table, T_LESS);
                    return t;
                }
            case LS_EQUAL:
                if (newChar == '=') {
                    t = ifj_generate_token(table, T_EQUAL);
                    return t;
                } else {
                    ungetc(newChar, l->inputFile);
                    t = ifj_generate_token(table, T_ASSIGN);
                    return t;
                }
            case LS_NEQ:
                if (newChar == '=') {
                    t = ifj_generate_token(table, T_NOT_EQUAL);
                    return t;
                } else {
                    ungetc(newChar, l->inputFile);
                    t = ifj_generate_token(table, T_NOT);
                    return t;
                }
            case LS_NUMBER_ZERO:
                if (newChar == 'b') {
                    state = LS_NUMBER_BIN;
                } else if (newChar == 'x') {
                    dyn_buffer_append(l->b_str, newChar);
                    state = LS_NUMBER_HEX;
                } else if (isdigit(newChar) || newChar == '.') {
                    ungetc(newChar, l->inputFile);
                    state = LS_NUMBER;
                } else {
                    ungetc(newChar, l->inputFile);
                    t = ifj_generate_token_int(table, 0);
                    return t;
                }
                break;
            case LS_NUMBER:
                if (isdigit(newChar)) {
                    dyn_buffer_append(l->b_str, newChar);
                    break;
                } else if (newChar == '.') {
                    dyn_buffer_append(l->b_str, newChar);
                    newChar = getc(l->inputFile);
                    if (!isdigit(newChar)) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                    ungetc(newChar, l->inputFile);
                    state = LS_DOUBLE_NUMBER;

                    break;
                } else if (newChar == 'e' || newChar == 'E') {
                    dyn_buffer_append(l->b_str, newChar);
                    state = LS_EXPO_FIRST_NUMBER;
                    break;
                } else if (newChar == '_') {
                    break;
                } else {
                    ungetc(newChar, l->inputFile);
                    long val = strtol(dyn_buffer_get_content(l->b_str), NULL,
                                      10);
                    if (errno == ERANGE) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                    dyn_buffer_clear(l->b_str);

                    if (val > INT_MAX) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }

                    t = ifj_generate_token_int(table, (int) val);
                    return t;
                }
                break;
            case LS_NUMBER_HEX:
                if (ishexadigit(newChar)) {
                    dyn_buffer_append(l->b_str, newChar);
                    break;
                } else if (newChar == '.') {
                    dyn_buffer_append(l->b_str, newChar);
                    newChar = getc(l->inputFile);
                    if (!isdigit(newChar)) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                    ungetc(newChar, l->inputFile);
                    state = LS_DOUBLE_NUMBER_HEX;
                    break;
                } else if (newChar == 'p' || newChar == 'P') {
                    dyn_buffer_append(l->b_str, newChar);
                    state = LS_EXPO_FIRST_NUMBER_HEX;
                    break;
                } else if (newChar == '_') {
                    break;
                } else {
                    ungetc(newChar, l->inputFile);
                    long val = strtol(dyn_buffer_get_content(l->b_str), NULL,
                                      16);
                    if (errno == ERANGE) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                    dyn_buffer_clear(l->b_str);

                    if (val > INT_MAX) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }

                    t = ifj_generate_token_int(table, (int) val);
                    return t;
                }
                break;
            case LS_NUMBER_BIN:
                if (newChar == '0' || newChar == '1') {
                    dyn_buffer_append(l->b_str, newChar);
                    break;
                } else if (newChar == '_') {
                    break;
                } else {
                    ungetc(newChar, l->inputFile);
                    long val = strtol(dyn_buffer_get_content(l->b_str), NULL,
                                      2);
                    if (errno == ERANGE) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                    dyn_buffer_clear(l->b_str);

                    if (val > INT_MAX) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }

                    t = ifj_generate_token_int(table, (int) val);
                    return t;
                }
            case LS_DOUBLE_NUMBER:
                if (isdigit(newChar)) {
                    dyn_buffer_append(l->b_str, newChar);
                    break;
                } else if (newChar == 'e' || newChar == 'E') {
                    dyn_buffer_append(l->b_str, 'e');
                    state = LS_EXPO_FIRST_NUMBER;
                    break;
                } else {
                    ungetc(newChar, l->inputFile);
                    double val = strtod(dyn_buffer_get_content(l->b_str), NULL);
                    dyn_buffer_clear(l->b_str);
                    if (errno == ERANGE) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                    t = ifj_generate_token_double(table, val);
                    return t;
                }
            case LS_EXPO_FIRST_NUMBER:
                if (newChar == '+' || newChar == '-') {
                    dyn_buffer_append(l->b_str, newChar);
                    newChar = getc(l->inputFile);
                    if (!isdigit(newChar)) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                    ungetc(newChar, l->inputFile);
                    state = LS_EXPO;
                    break;
                } else if (isdigit(newChar)) {
                    dyn_buffer_append(l->b_str, newChar);
                    state = LS_EXPO;
                    break;
                } else {
                    l->inter->returnCode = 1;
                    t = ifj_generate_token(table, T_UNKNOWN);
                    return t;
                }
                break;
            case LS_EXPO:
                if (isdigit(newChar)) {
                    dyn_buffer_append(l->b_str, newChar);
                    break;
                } else {
                    ungetc(newChar, l->inputFile);
                    double val = strtod(dyn_buffer_get_content(l->b_str), NULL);
                    dyn_buffer_clear(l->b_str);
                    if (errno == ERANGE) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                    t = ifj_generate_token_double(table, val);
                    return t;
                }
                break;
            case LS_WORD:
                if (isalnum(newChar) || newChar == '_' || newChar == '$') {
                    dyn_buffer_append(l->b_str, newChar);
                    break;
                } else if (newChar == '.') {
                    dyn_buffer_append(l->b_str, newChar);
                    newChar = getc(l->inputFile);
                    if (!isalnum(newChar)) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                    ungetc(newChar, l->inputFile);
                    state = LS_WORD_D;
                    break;
                } else {
                    char *value = dyn_buffer_get_content(l->b_str);
                    int tokenType = ifj_lexa_is_reserved(l, value);

                    ungetc(newChar, l->inputFile);

                    if (tokenType != -1) {
                        t = ifj_generate_token(table, tokenType);
                        return t;
                    } else {
                        t = ifj_generate_token_id(
                                dyn_buffer_get_content(l->b_str));
                        return t;
                    }
                }
                break;
            case LS_WORD_D:
                if (isalnum(newChar) || newChar == '_') {
                    dyn_buffer_append(l->b_str, newChar);
                    break;
                } else {
                    char *value = dyn_buffer_get_content(l->b_str);
                    int tokenType = ifj_lexa_is_reserved(l, value);

                    ungetc(newChar, l->inputFile);

                    if (tokenType != -1) {
                        t = ifj_generate_token(table, tokenType);
                        return t;
                    } else {
                        t = ifj_generate_token_id(
                                dyn_buffer_get_content(l->b_str));
                        return t;
                    }
                }
                break;
            case LS_DOUBLE_NUMBER_HEX:
                if (ishexadigit(newChar)) {
                    dyn_buffer_append(l->b_str, newChar);
                    break;
                } else if (newChar == 'p' || newChar == 'P') {
                    dyn_buffer_append(l->b_str, 'p');
                    state = LS_EXPO_FIRST_NUMBER_HEX;
                    break;
                } else {
                    ungetc(newChar, l->inputFile);
                    double val = strtod(dyn_buffer_get_content(l->b_str), NULL);
                    dyn_buffer_clear(l->b_str);
                    if (errno == ERANGE) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                    t = ifj_generate_token_double(table, val);
                    return t;
                }
                break;
            case LS_EXPO_FIRST_NUMBER_HEX:
                if (newChar == '+' || newChar == '-') {
                    dyn_buffer_append(l->b_str, newChar);
                    newChar = getc(l->inputFile);
                    if (!ishexadigit(newChar)) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                    ungetc(newChar, l->inputFile);
                    state = LS_EXPO_HEX;
                    break;
                } else {
                    l->inter->returnCode = 1;
                    t = ifj_generate_token(table, T_UNKNOWN);
                    return t;
                }
                break;
            case LS_EXPO_HEX:
                if (ishexadigit(newChar)) {
                    dyn_buffer_append(l->b_str, newChar);
                    break;
                } else {
                    ungetc(newChar, l->inputFile);
                    double val = strtod(dyn_buffer_get_content(l->b_str), NULL);
                    dyn_buffer_clear(l->b_str);
                    if (errno == ERANGE) {
                        l->inter->returnCode = 1;
                        t = ifj_generate_token(table, T_UNKNOWN);
                        return t;
                    }
                    t = ifj_generate_token_double(table, val);
                    return t;
                }
                break;
        }
    }

}
