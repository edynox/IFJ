/* ial.c
 *
 * Copyright (C) 2016 SsYoloSwag41 Inc.
 * Authors: Eduard Cuba <xcubae00@stud.fit.vutbr.cz>
 */

#include "ial.h"
#include <stdlib.h>
#include <stdio.h>

#define TABLE_ROWS 4241

/**
 * Add new item
 * - store pointer to token in hash table
 * - allows to access pointer later
 */
const token *ial_symbol_table_add_item	( 	symbol_table *self,
	 										token *item )
{
	return NULL;
}

/**
 * sdbm hashing algorithm
 */
unsigned int ial_symbol_table_hash_func ( token *item )
{
	if (!item->name)
		return -1;

	unsigned int hash = 0;
	unsigned char *begin = (unsigned char*)item->name;
	int current;
	while( (current = *begin) )
	{
		begin++;
		hash = current + (hash << 6) + (hash << 16) - hash;
	}
	printf("Hash:%u\n",hash);
	return hash % TABLE_ROWS;
}

/**
 * Symbol table constructor
 * @return new uninitialised symbol_table
 */
static symbol_table* ial_symbol_table_construct()
{
    symbol_table *table = (symbol_table*) malloc ( sizeof( struct _symbol_table ));
    if(!table)
    {
        fprintf(stderr,"ERROR: allocating symbol table structure!\n");
        exit(99);
    }
	table->size = TABLE_ROWS;
	table->row = (token**) malloc ( sizeof( token*) * table->size );
	if(!table->row)
    {
        fprintf(stderr,"ERROR: allocating symbol table!\n");
        exit(99);
    }
    return table;
}

/**
 * Symbol table initializer
 * @param self symbol_table
 */
static void ial_symbol_table_init( symbol_table *self )
{
	self->hash_func	= &ial_symbol_table_hash_func;
	self->add_item	= &ial_symbol_table_add_item;
	for( int i = 0; i < self->size; i++)
	{
		self->row[i] = NULL;
	}
}

/**
 * Create new symbol table structure
 *  - contains default methods + hash structure
 * @return  new symbol table
 */
symbol_table *ial_symbol_table_new()
{
    symbol_table *table = ial_symbol_table_construct();
    ial_symbol_table_init(table);
    return table;
}
