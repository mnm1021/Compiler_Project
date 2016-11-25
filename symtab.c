/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

/* the hash function */
static int hash ( char * key )
{
    int ret = 0;
    int i = 0;
    
    while (key[i] != '\0')
    {
        ret = ((ret << SHIFT) + key[i]) % SIZE;
        ++i;
    }
    
    return ret;
}

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 */
int st_insert( BucketList *hashTable, TreeNode *t, int is_function )
{
    int h = hash(t->attr.name);
    BucketList l =  hashTable[h];
    
    while ( (l != NULL) && (strcmp(t->attr.name,l->name) != 0) )
    {
        l = l->next;
    }
    
    if (l == NULL) /* variable not yet in table */
    {
        l = (BucketList) malloc(sizeof(struct BucketListRec));
        l->name = t->attr.name;
        l->lineno = t->lineno;
        l->is_function = is_function;
        l->type = t->type;
        l->param = NULL;

        l->next = hashTable[h];
        hashTable[h] = l;

        return 0; /* success */
    }
    else
    {
        return -1; /* duplicated declaration */
    }
} /* st_insert */

/**
 * st_lookup calls table_lookup recursively finds the variable from bottom to up.
 */
BucketList st_lookup ( struct SymbolTable *table, char * name )
{
    BucketList lookup_result = table_lookup( table->hashTable, name );

    if (lookup_result != NULL)
    {
        return lookup_result;
    }
    else
    {
        if (table->parent == NULL)
        {
            /* reached over global scope : return -1. */
            return NULL;
        }
        else
        {
            return st_lookup( table->parent, name );
        }
    }
}

/* Function table_lookup returns 0 if found, or -1 if not found
 */
BucketList table_lookup ( BucketList *hashTable, char * name )
{
    int h = hash(name);
    BucketList l =  hashTable[h];

    while ( (l != NULL) && (strcmp(name,l->name) != 0) )
    {
        l = l->next;
    }

    return l; /* l can be NULL if there is no variable found. */
}

static void printType(Type type)
{
    if (type == Integer)
    {
        printf("Integer\n");
    }
    else if (type == Void)
    {
        printf("Void\n");
    }
    else if (type == IntegerArray)
    {
        printf("IntegerArray\n");
    }
}

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing)
{
    int i;
    struct SymbolTable *currentTable;
    BucketList bucket, paramBucket;

    /* print function declarations */
    printf("<FUNCTION DECLARATIONS>\n");

    currentTable = globalTable;
    for (i = 0; i < SIZE; i++)
    {
        bucket = currentTable->hashTable[i];

        while (bucket != NULL)
        {
            if (bucket->is_function)
            {
                /* print each function */
                printf("Function Name     Data Type\n");
                printf("-------------   -------------\n");
                printf("%-17s", bucket->name);
                printType(bucket->type);

                /* print every params */
                printf("Function Parameters     Data Type\n");
                printf("-------------------   -------------\n");

                paramBucket = bucket->param;
                if (paramBucket == NULL)
                {
                    printf("%-23s%s\n", "Void", "Void");
                }
                else
                {
                    while (paramBucket != NULL)
                    {
                        printf("%-23s",paramBucket->name);
                        printType(paramBucket->type);
                        paramBucket = paramBucket->param;
                    }
                }

                printf("\n");
            }

            bucket = bucket->next;
        }
    }

    /* */

} /* printSymTab */































