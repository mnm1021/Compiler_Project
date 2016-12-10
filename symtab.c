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
int st_insert( BucketList *hashTable, TreeNode *t, int is_function,
       int location, int is_global, int is_param )
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

        l->is_param = is_param;
        l->is_global = is_global;
        l->location = location;

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

/**
 * print given type into string.
 */
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

/**
 * find table that has the given order.
 */
struct SymbolTable *findNewTableInOrder(
        struct SymbolTable *currentTable,
        int order)
{
    struct SymbolTable *childTable, *resultTable;

    if (currentTable != NULL)
    {
        if (currentTable->order == order)
        {
            return currentTable;
        }

        /* search through children. */
        childTable = currentTable->child;
        while (childTable != NULL)
        {
            resultTable = findNewTableInOrder(childTable, order);
            if (resultTable != NULL)
            {
                return resultTable;
            }

            childTable = childTable->child;
        }

        /* search through siblings. */
        return findNewTableInOrder(currentTable->sibling, order);
    }
    
    return NULL;
}

/**
 * print function scope recursively.
 */
static void printFunctionScope(struct SymbolTable *currentTable,
                               struct SymbolTable *baseTable)
{
    BucketList bucket;
    int i;

    if (currentTable != NULL)
    {
        /* print header of table. */
        printf("function name : %s (nested level : %d)\n",
                currentTable->functionName,
                currentTable->depth);
        printf("   ID NAME        ID TYPE        DATA TYPE\n");
        printf("-------------  -------------   --------------\n");

        for (i = 0; i < SIZE; i++)
        {
            bucket = currentTable->hashTable[i];

            while (bucket != NULL)
            {
                printf("%-15s", bucket->name);

                if (bucket->is_function)
                {
                    printf("%-16s", "Function");
                }
                else
                {
                    printf("%-16s", "Variable");
                }

                printType(bucket->type);

                bucket = bucket->next;
            }
        }
        printf("\n");

        /* find new scope's table in order. */
        printFunctionScope(findNewTableInOrder(baseTable->child,
                                               currentTable->order + 1),
                           baseTable);
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
                printf("%-16s", bucket->name);
                printType(bucket->type);

                /* print every params */
                printf("Function Parameters     Data Type\n");
                printf("-------------------   -------------\n");

                paramBucket = bucket->param;
                if (paramBucket == NULL)
                {
                    printf("%-22s%s\n", "Void", "Void");
                }
                else
                {
                    while (paramBucket != NULL)
                    {
                        printf("%-22s",paramBucket->name);
                        printType(paramBucket->type);
                        paramBucket = paramBucket->param;
                    }
                }

                printf("\n");
            }

            bucket = bucket->next;
        }
    }

    /* print functions and global variables */
    printf("<FUNCTION AND GLOBAL VARIABLES>\n");
    printf("   ID NAME        ID TYPE        DATA TYPE\n");
    printf("-------------  -------------   --------------\n");
    
    for (i = 0; i < SIZE; i++)
    {
        bucket = currentTable->hashTable[i];

        while (bucket != NULL)
        {
            printf("%-15s", bucket->name);

            if (bucket->is_function)
            {
                printf("%-16s", "Function");
            }
            else
            {
                printf("%-16s", "Variable");
            }

            printType(bucket->type);

            bucket = bucket->next;
        }
    }

    /* print each function's parameter and local variables. */
    printf("\n<FUNCTION PARAMETERS AND LOCAL VARIABLES>\n");
    
    currentTable = globalTable->child;
    while (currentTable != NULL)
    {
        printFunctionScope(currentTable, currentTable);

        currentTable = currentTable->sibling;
    }

} /* printSymTab */
