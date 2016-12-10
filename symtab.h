/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

/* SIZE is the size of the hash table */
#define SIZE 211

/* NAME_LENGTH is the max length of function name. */
#define NAME_LENGTH 100

#include "globals.h"


/* The record in the bucket lists for
 * each variable, including name, 
 * assigned memory location, and
 * the list of line numbers in which
 * it appears in the source code
 */
typedef struct BucketListRec
{
    char * name;
    int lineno;
    int is_function;
    Type type;
    struct BucketListRec * next;
    struct BucketListRec * param;

    int is_param;
    int is_global;
    int location; /* address that this symbol is stored in */
} *BucketList;

/**
 * struct SymbolTable represents a symbol table:
 * an instance of SymbolTable represents a scope of a single block.
 */
struct SymbolTable
{
    /* hashTable has the data of whole declarations. */
    BucketList hashTable[SIZE];

    /* functionName has the name of function. global is set to __GLOBAL__. */
    char functionName[NAME_LENGTH];

    /* depth represents nested level. */
    int depth;

    /* children has the nested block's scope. */
    struct SymbolTable *child;

    /* sibling has the nested block's scope of parent.
       this works like a linked list for parent. */
    struct SymbolTable *sibling;

    /* parent represents the parent scope of this block. */
    struct SymbolTable *parent;

    /* represents that this symbol table is already visited when traversing. */
    int visited;

    /* table traversing order. */
    int order;
};

/* represents global scope. */
struct SymbolTable *globalTable;

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 */
int st_insert( BucketList *hashTable, TreeNode *t, int is_function,
        int location, int is_global, int is_param );

/**
 * st_lookup calls table_lookup recursively finds the variable from bottom to up.
 */
BucketList st_lookup ( struct SymbolTable *table, char * name );

/* Function table_lookup returns 0 if found, or -1 if not found
 */
BucketList table_lookup ( BucketList *hashTable, char * name );

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing);

/**
 * find table that has the given order.
 */
struct SymbolTable *findNewTableInOrder(
        struct SymbolTable *currentTable,
        int order);

#endif
