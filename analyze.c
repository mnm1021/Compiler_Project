/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"

/* counter for variable memory locations */
static int location = 0;

/* represents current symbol table. */
struct SymbolTable *currentTable;

/* variable for not duplicating child. */
int isNewFunctionDeclared = 0;

/* set traversing order. */
int order = 0;

/******************
 * error handlers *
 ******************/

static void duplicateError(TreeNode * t, char * message)
{
    fprintf(listing,"error : already declared variable %s at line %d\n", message, t->lineno);
    Error = TRUE;
}

static void typeError(TreeNode * t)
{
    fprintf(listing,"error : type inconsistance at line %d\n", t->lineno);
    Error = TRUE;
}

static void voidVariableError(TreeNode * t)
{
    fprintf(listing,"error : Variable type cannot be Void at line %d\n", t->lineno);
    Error = TRUE;
}

static void returnTypeError(TreeNode * t)
{
    fprintf(listing,"type error at line %d : return type inconsistance\n",t->lineno);
    Error = TRUE;
}

static void undeclaredVariableError(TreeNode * t, char *name)
{
    fprintf(listing,"error : undeclared variable %s at line %d\n", name, t->lineno);
    Error = TRUE;
}

static void undeclaredFunctionError(TreeNode * t, char *name)
{
    fprintf(listing,"error : undeclared function %s at line %d\n", name, t->lineno);
    Error = TRUE;
}

static void invalidFunctionError(TreeNode * t)
{
    fprintf(listing,"type error at line %d : invalid function call\n", t->lineno);
    Error = TRUE;
}

/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t,
               void (* preProc) (TreeNode *),
               void (* postProc) (TreeNode *) )
{
    if (t != NULL)
    {
        preProc(t);

        /* traverse through children. */
        int i;
        for (i=0; i < MAXCHILDREN; i++)
        {
            traverse(t->child[i],preProc,postProc);
        }
        
        postProc(t);

        traverse(t->sibling,preProc,postProc);
    }
}

/* backtrack if compound statement ended.
 */
static void backtrackProc(TreeNode * t)
{
    /* backtrack to parent if current node is compound statement. */
    if (t->nodekind == StmtK && t->kind.stmt == CompoundStmt)
    {
        currentTable = currentTable->parent;
    }
}

/* go forward if function or compound statement starts.
 */
static void forwardProc(TreeNode * t)
{
    if (t->nodekind == DeclareK
            && t->kind.declaration == IdDec
            && t->child[1] != NULL
            && t->child[1]->nodekind == StmtK)
    {
        currentTable = currentTable->child;

        /* find the following function name from siblings. */
        while (currentTable != NULL
                && strcmp(currentTable->functionName, t->attr.name) != 0)
        {
            currentTable = currentTable->sibling;
        }

        isNewFunctionDeclared = 1;
    }
    else if (t->nodekind == StmtK)
    {
        if (t->kind.stmt == CompoundStmt)
        {
            if (isNewFunctionDeclared)
            {
                isNewFunctionDeclared = 0;
            }
            else
            {
                currentTable = currentTable->child;

                /* find the following function name from siblings. */
                while (currentTable != NULL && currentTable->visited != 0)
                {
                    currentTable = currentTable->sibling;
                }

                currentTable->visited = 1;
            }
        }
    }
}

static int paramCheck(BucketList lookupResult, TreeNode *t)
{
    lookupResult = lookupResult->param;
    t = t->child[0];

    while (lookupResult != NULL || t != NULL)
    {
        /* number of parameter mismatch */
        if (lookupResult == NULL || t == NULL)
        {
            return 0;
        }

        /* type mismatch */
        if (lookupResult->type != t->type)
        {
            return 0;
        }

        lookupResult = lookupResult->param;
        t = t->sibling;
    }

    return 1;
}

/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */
static void insertNode( TreeNode * t)
{
    /* only add to table if declaration. */
    if (t->nodekind == DeclareK)
    {
        if (t->kind.declaration == IdDec)
        {
            int isFunction;

            /* figure out this is function or variable */
            if (t->child[1] != NULL && t->child[1]->nodekind == StmtK)
            {
                isFunction = 1;

                if (st_insert( currentTable->hashTable, t, isFunction ) == -1)
                {
                    /* error : duplicated insertion */
                    duplicateError(t, t->attr.name);
                }

                /* create and initialize new table. */
                struct SymbolTable *newTable
                    = (struct SymbolTable *)malloc(sizeof(struct SymbolTable));
            
                strncpy( newTable->functionName, 
                         t->attr.name, 
                         strlen(t->attr.name) + 1);
                newTable->depth = currentTable->depth + 1;
                newTable->child = NULL;
                newTable->sibling = NULL;
                newTable->parent = currentTable;
                newTable->visited = 0;
                newTable->order = order;
                order++;

                /* add to currentTable's child. */
                struct SymbolTable *childTable = currentTable->child;
                if (childTable == NULL)
                {
                    currentTable->child = newTable;
                }
                else
                {
                    while (childTable->sibling != NULL)
                    {
                        childTable = childTable->sibling;
                    }
                    childTable->sibling = newTable;
                }

                /* set new table to current. */
                currentTable = newTable;

                isNewFunctionDeclared = 1;
            }
            else
            {
                isFunction = 0;
                if (t->type == Void || t->type == VoidArray)
                {
                    voidVariableError(t);
                }

                if (st_insert( currentTable->hashTable, t, isFunction ) == -1)
                {
                    /* error : duplicated insertion */
                    duplicateError(t, t->attr.name);
                }
            }
        }
        else if (t->kind.declaration == ParamDec)
        {
            if (st_insert( currentTable->hashTable, t, 0 ) == -1)
            {
                /* error : duplicated insertion */
                duplicateError(t, t->attr.name);
            }

            /* add param to function. */
            BucketList node = st_lookup( globalTable, currentTable->functionName );
            if (node != NULL)
            {
                while (node->param != NULL)
                {
                    node = node->param;
                }

                node->param = (BucketList)malloc(sizeof(*node));
                node->param->name = t->attr.name;
                node->param->type = t->type;
                node->param->param = NULL;
                node->param->lineno = t->lineno;
            }
        }
    }
    /* check if the ID is in table. */
    else if (t->nodekind == ExpK)
    {
        BucketList lookupResult;

        switch (t->kind.exp)
        {
            case OpExp:
                if (t->attr.op != ASSIGN)
                {
                    t->type = Integer;
                }

                break;

            case ConstExp:
                t->type = Integer;
                break;

            case IdExp:
                if ((lookupResult = st_lookup( currentTable, t->attr.name )) != NULL)
                {
                    if (lookupResult->type == IntegerArray && t->child[0] != NULL)
                    {
                        t->type = Integer;
                    }
                    else
                    {
                        t->type = lookupResult->type;
                    }
                }
                else
                {
                    if (t->type == Func)
                    {
                        undeclaredFunctionError(t, t->attr.name);
                    }
                    else
                    {
                        undeclaredVariableError(t, t->attr.name);
                    }
                }
                break;

            default:    
                break;
        }
    }
    /* set new scope for compound statement. */
    else if (t->nodekind == StmtK)
    {
        if (t->kind.stmt == CompoundStmt)
        {
            /* do not add new scope if new function is declared:
               this means that this compound statement is the
               main body of function, which is in depth 1. */
            if (isNewFunctionDeclared)
            {
                isNewFunctionDeclared = 0;
            }
            /* if isNewFunctionDeclared is already 0,
               set scope for nested block. */
            else
            {
                /* create and initialize new table. */
                struct SymbolTable *newTable 
                    = (struct SymbolTable *)malloc(sizeof(struct SymbolTable));
            
                strncpy( newTable->functionName, 
                         currentTable->functionName, 
                         strlen(currentTable->functionName) + 1);
                newTable->depth = currentTable->depth + 1;
                newTable->child = NULL;
                newTable->sibling = NULL;
                newTable->parent = currentTable;
                newTable->visited = 0;
                newTable->order = order;
                order++;

                /* add to currentTable's child. */
                struct SymbolTable *childTable = currentTable->child;
                if (childTable == NULL)
                {
                    currentTable->child = newTable;
                }
                else
                {
                    while (childTable->sibling != NULL)
                    {
                        childTable = childTable->sibling;
                    }
                    childTable->sibling = newTable;
                }

                /* set new table to current. */
                currentTable = newTable;
            }
        }
    }
}

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{
    /* create global table. */
    globalTable = (struct SymbolTable *)malloc(sizeof(struct SymbolTable));

    /* initialize global table. */
    strncpy(globalTable->functionName, "__GLOBAL__", 11);
    globalTable->depth = 0;
    globalTable->child = NULL;
    globalTable->sibling = NULL;
    globalTable->parent = NULL;
    globalTable->visited = 1;
    globalTable->order = order;
    order++;

    currentTable = globalTable;

    /******************************************/
    /* add builtin functions. */
    /******************************************/
    TreeNode *inputNode = (TreeNode *)malloc(sizeof(TreeNode));

    inputNode->attr.name = "input";
    inputNode->lineno = 0;
    inputNode->type = Integer;

    st_insert(globalTable->hashTable, inputNode, 1);

    TreeNode *outputNode = (TreeNode *)malloc(sizeof(TreeNode));

    outputNode->attr.name = "output";
    outputNode->lineno = 0;
    outputNode->type = Void;

    st_insert(globalTable->hashTable, outputNode, 1);

    /* add param to output. */
    BucketList outputHash = st_lookup(globalTable, "output");
    outputHash->param = (BucketList)malloc(sizeof(*outputHash));
    outputHash->param->type = Integer;
    outputHash->param->name = "arg";

    /*******************************************/

    /* traverse through syntax tree, build symbol table. */
    traverse(syntaxTree, insertNode, backtrackProc);

    /* print whole symbol table. */
    if (TraceAnalyze)
    {
        fprintf(listing,"\nSymbol table:\n\n");
        printSymTab(listing);
    }
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t)
{
    Type returnType;
    BucketList lookupResult;

    switch (t->nodekind)
    {
        case StmtK:
            switch (t->kind.stmt)
            {
                case ReturnStmt:
                    if (t->child[0] == NULL)
                    {
                        returnType = Void;
                    }
                    else
                    {
                        returnType = t->child[0]->type;
                    }

                    lookupResult = st_lookup( globalTable,
                                              currentTable->functionName );
                    if (lookupResult == NULL
                            || returnType != lookupResult->type)
                    {
                        returnTypeError( t );
                    }

                    break;
                
                case CompoundStmt:
                    /* return to parent symbol table if compound statement ended. */
                    currentTable = currentTable->parent;
                    break;

                default:
                    break;
            }

            break;

        case ExpK:
            /* check the validity of assignment */
            if (t->kind.exp == OpExp)
            {
                if (t->attr.op == ASSIGN)
                {
                    if (t->child[0]->type != t->child[1]->type)
                    {
                        typeError( t );
                    }
                    else
                    {
                       t->type = t->child[0]->type;
                    }
                }
            }
            else if (t->kind.exp == IdExp)
            {
                lookupResult = st_lookup(currentTable, t->attr.name);
                    
                /* check parameters if function */
                if (lookupResult != NULL)
                {
                    if (lookupResult->is_function)
                    {
                        if (paramCheck(lookupResult, t) == 0)
                        {
                            invalidFunctionError( t );
                        }
                    }
                }
            }
            
            break;

        default:
            break;
    }
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{
    traverse(syntaxTree, forwardProc, checkNode);
}
