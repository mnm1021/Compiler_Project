/****************************************************/
/* File: cgen.c                                     */
/* The code generator implementation                */
/* for the TINY compiler                            */
/* (generates code for the TM machine)              */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "code.h"
#include "cgen.h"

#define MAX_SYMBOL 1000

/*
 * function locations.
 */
static int functionLocations[MAX_SYMBOL];

/*
 * offset for global poiner / stack pointer.
 */
static int globalOffset = 0;
static int localOffset = 0;

/*
 * current symbol table, searching order.
 */
static struct SymbolTable *currentTable;
static int order = 0;

/*
 * location of main function.
 */
static int mainFunctionLoc;

/* prototype for internal recursive code generator */
static void cGen (TreeNode * tree);

/* Procedure genDeclare generates code at a declaration node */
static void genDeclare( TreeNode * tree)
{
    static struct SymbolTable *tmpTable;
    BucketList node;
    int loc, currentLoc;
    int size;

    switch(tree->kind.declaration)
    {
        /* name of variable or function. */
        case IdDec:
            /* function */
            if (tree->child[1] != NULL && tree->child[1]->nodekind == StmtK)
            {
                /* increment global offset. */
                globalOffset += 1;

                /* set tmpTable for parameters. */
                tmpTable = currentTable->child;
                while (strcmp(tmpTable->functionName, tree->attr.name) != 0)
                {
                    tmpTable = tmpTable->sibling;
                }

                /* get location of function. */
                loc = st_lookup(currentTable, tree->attr.name)->location;

                /* store current location to array. */
                currentLoc = emitSkip(0);
                functionLocations[loc] = currentLoc;

                /* store main function's location. */
                if (strcmp(tree->attr.name, "main") == 0)
                {
                    mainFunctionLoc = currentLoc;
                }

                /* push previous frame pointer address. */
                emitRM("ST", fp, -2, mp, "store previous frame pointer address.");

                /* set frame pointer. */
                emitRM_Abs("LDA", ac, 3, "load value 3 to ac.");
                emitRO("SUB", fp, mp, ac, "fp = mp - 3");
                emitRO("SUB", mp, mp, ac, "mp = mp - 3");

                /* generate code of current function :
                   calculate memory for parameters.
                   setting stack pointer would be done in next phase.*/
                cGen(tree->child[0]);

                /* generate code of current function :
                   handle compound statements.
                   this would calculate local variables, and set stack pointer. */
                cGen(tree->child[1]);

                /* create return instruction :
                   do not use ac, since it has return value. */
                emitComment("Return Statements.");
                emitRM_Abs("LDA", ac1, 3, "load value 3 to ac1.");
                emitRO("ADD", mp, fp, ac1, "mp = fp + 3");
                emitRM("LD", fp, 1, fp, "set fp to previous frame pointer.");
                emitRM("LD", ac1, -1, mp, "set ac1 to previous address.");
                emitRO("ADD", pc, ac1, constant, "pc = previous address + 1");
                emitComment("Return Statements ended.");
            }
            /* variable */
            else
            {
                node = st_lookup(currentTable, tree->attr.name);
                if (node != NULL)
                {
                    if (node->type == IntegerArray)
                    {
                        size = tree->child[0]->attr.val;
                    }
                    else
                    {
                        size = 1;
                    }

                    /* increment offset. */
                    if (node->is_global)
                    {
                        globalOffset += size;
                    }
                    else
                    {
                        localOffset += size;
                    }
                }
                else
                {
                    /* unknown variable error */
                }
            }
            
            break;

        /* name of parameter variable. */
        case ParamDec:
            node = st_lookup(tmpTable, tree->attr.name);
            if (node != NULL)
            {
                /* handle array as reference, so size is 1. */
                localOffset += 1;
            }
            else
            {
                /* unknown parameter error */
            }
            break;

        default:
            /* unknown node error */
            break;
    }
} /* genDeclare */

/* Procedure genStmt generates code at a statement node */
static void genStmt( TreeNode * tree)
{
    int offset;
    int firstLoc, secondLoc, currentLoc;
    int firstBlock, secondBlock;

    switch(tree->kind.stmt)
    {
        /* left(child[0]) : local_declarations
           right(child[1]) : statement_list */
        case CompoundStmt:
            /* set currentTable to next table. */
            order += 1;
            currentTable = findNewTableInOrder(globalTable, order);

            /* calculate local offset. */
            cGen(tree->child[0]);

            /* store current localOffset. */
            offset = localOffset;

            /* set stack pointer. */
            emitRM_Abs("LDA", ac, offset, "load size of local vars to ac.");
            emitRO("SUB", mp, mp, ac, "mp = mp - localOffset");

            /* set local offset into 0, since setting stack pointer is finished. */
            localOffset = 0;

            /* generate code for statements. */
            cGen(tree->child[1]);

            /* reset stack pointer since compound statement has ended. */
            emitRM_Abs("LDA", ac1, offset, "load size of local vars to ac1.");
            emitRO("ADD", mp, mp, ac1, "mp = mp + localOffset");

            /* restore table. */
            currentTable = currentTable->parent;
            break;

        /* left(child[0]) : expression */
        /* middle(child[1]) : statement(includes every kind of statement) */
        /* right(child[2]) : statement or NULL. [1] is inside if, [2] is else. */
        case SelectionStmt:
            /* generate code for expression. */
            cGen(tree->child[0]);

            /* save current location. */
            firstLoc = emitSkip(2);

            /* generate code for statements in 'if'. */
            firstBlock = emitSkip(0);
            cGen(tree->child[1]);

            /* generate code for statements in 'else'.
               if no 'else', secondBlock would point to the next statement. */
            secondBlock = emitSkip(0);
            if (tree->child[2] != NULL && tree->child[2]->nodekind != EmptyK)
            {
                secondLoc = emitSkip(1);
                secondBlock = emitSkip(0);

                cGen(tree->child[2]);
                currentLoc = emitSkip(0);

                /* make jump for 'if' statement. */
                emitBackup(secondLoc);
                emitRM_Abs("JEQ", zero, currentLoc, "jump to nonconditional area.");
            }

            /* set jump condition to restored area. */
            emitBackup(firstLoc);
            emitRM_Abs("JEQ", ac, firstBlock, "jump to firstBlock if ac == 0.");
            emitRM_Abs("JNE", ac, secondBlock, "jump to secondBlock if ac != 0.");

            /* restore location. */
            emitRestore();
            break;

        /* left(child[0]) : expression */
        /* right(child[1]) : statement */
        case IterationStmt:
            /* save the start of loop block. */
            firstBlock = emitSkip(0);

            /* generate code for expresion. */
            cGen(tree->child[0]);

            /* save current location. */
            firstLoc = emitSkip(1);

            /* generate code for statements in 'while'. */
            cGen(tree->child[1]);

            /* add non-conditional jump for loop. */
            emitRM_Abs("JEQ", zero, firstBlock, "loop of firstBlock.");

            /* get current location. */
            secondBlock = emitSkip(0);

            /* set jump condition to restored area. */
            emitBackup(firstLoc);
            emitRM_Abs("JNE", ac, secondBlock, "jump to secondBlock if ac != 0.");

            /* restore location. */
            emitRestore();
            break;

        /* child[0] : expression or NULL */
        case ReturnStmt:
            /* generate code for expression. */
            cGen(tree->child[0]);
            
            /* returned value is already in register 'ac'. */
            break;

        default:
            /* unknown node error */
            break;
    }
} /* genStmt */

/* Procedure genExp generates code at an expression node */
static void genExp( TreeNode * tree)
{
    BucketList var;
    int location;
    TreeNode *param, *left;
    int offset;

    int firstLoc, secondLoc;

    switch(tree->kind.exp)
    {
        /* if ASSIGN : left is var, right is expression. */
        /* else : left and right both expression. */
        case OpExp:
            /* get expression value from right. */
            cGen(tree->child[1]);

            /* store right expression value on stack. */
            emitRM("ST", ac, -1, mp, "mem[mp - 1] = right expression");

            /* push mp 1. */
            emitRO("SUB", mp, mp, constant, "mp = mp - 1");

            /* if not assign, get left value and store to ac. */
            if (tree->attr.op != ASSIGN)
            {
                /* get expression value from left. */
                cGen(tree->child[0]);

                /* pop mp 1. */
                emitRO("ADD", mp, mp, constant, "mp = mp + 1");

                /* load right expression value to ac1. */
                emitRM("LD", ac1, -1, mp, "ac1 = mem[mp - 1]");
            }

            /* handle according to the values. */
            switch(tree->attr.op)
            {
                case ASSIGN:
                    left = tree->child[0];
                    var = st_lookup(currentTable, left->attr.name);
                    location = 0 - var->location;

                    /* array : handle reference. */
                    if (var->type == IntegerArray)
                    {
                        /* if pointing array itself, resolve reference. */
                        if (left->child[0] == NULL)
                        {
                            /* pop mp 1. */
                            emitRO("ADD", mp, mp, constant, "mp = mp + 1");

                            /* load right expression value to ac1. */
                            emitRM("LD", ac1, -1, mp, "ac1 = mem[mp - 1]");

                            /* C-Minus do not support pointer expression,
                               so this would be left in blank.
                               just resolved mp to original state. */
                        }
                        /* if pointing array's element, calculate offset. */
                        else
                        {
                            /* generate expression code for offset.
                               register 'ac' has the offset. */
                            cGen(left->child[0]);

                            /* if parameter. resolve reference. */
                            if (var->is_param == 1)
                            {
                                /* register 'ac' has the index. */

                                emitRM("LD", ac1, location, fp,
                                        "load reference to ac1.");
                                emitRO("SUB", ac1, ac1, ac, "ac1 = ac1 - ac");

                                /* store ac1's value to ac. */
                                emitRO("ADD", ac, ac1, zero, "ac = ac1");

                                /* pop mp 1. */
                                emitRO("ADD", mp, mp, constant, "mp = mp + 1");

                                /* load right expression value to ac1. */
                                emitRM("LD", ac1, -1, mp, "ac1 = mem[mp - 1]");

                                /* store ac1's value to stack. */
                                emitRM("ST", ac1, 0, ac, "memory[ac] = ac1");
                            }
                            /* else, calculate offset. */
                            else
                            {
                                /* if global array, calculate from gp. */
                                if (var->is_global == 1)
                                {
                                    emitRO("SUB", ac, gp, ac, "ac = gp - offset");
                                }
                                /* else, calculate from fp. */
                                else
                                {
                                    emitRO("SUB", ac, fp, ac, "ac = fp - offset");
                                }

                                /* pop mp 1. */
                                emitRO("ADD", mp, mp, constant, "mp = mp + 1");

                                /* load right expression value to ac1. */
                                emitRM("LD", ac1, -1, mp, "ac1 = mem[mp - 1]");

                                /* store right expression value to stack. */
                                emitRM("ST", ac1, location, ac,
                                        "memory[ac - location] = ac1");
                            }
                        }
                    }
                    /* plain variable :
                       get reference by frame/global pointer. */
                    else
                    {
                        /* pop mp 1. */
                        emitRO("ADD", mp, mp, constant, "mp = mp + 1");

                        /* load right expression value to ac1. */
                        emitRM("LD", ac1, -1, mp, "ac1 = mem[mp - 1]");
                        
                        /* global variable : subtract from global pointer. */
                        if (var->is_global == 1)
                        {
                            emitRM("ST", ac1, location, gp,
                                    "memory[gp - location] = ac1");
                        }
                        /* local variable : subtract from frame pointer. */
                        else
                        {
                            emitRM("ST", ac1, location, fp,
                                    "memory[fp - location] = ac1");
                        }
                    }
                    break;

                case PLUS:
                    emitRO("ADD", ac, ac, ac1, "ac = ac + ac1");
                    break;

                case MINUS:
                    emitRO("SUB", ac, ac, ac1, "ac = ac - ac1");
                    break;

                case TIMES:
                    emitRO("MUL", ac, ac, ac1, "ac = ac * ac1");
                    break;

                case OVER:
                    emitRO("DIV", ac, ac, ac1, "ac = ac / ac1");
                    break;

                case EQ:
                    emitRO("SUB", ac, ac, ac1, "operator == : ac == 0 if equal");
                    break;

                case NE:
                    emitRO("SUB", ac, ac, ac1, "op !=");
                    emitRM("JNE", ac, 2, pc, "jump if true");
                    emitRO("ADD", ac, constant, zero, "a = 1 : not true");
                    emitRM("JEQ", zero, 1, pc, "jump to next instruction");
                    emitRO("ADD", ac, zero, zero, "a = 0 : true");
                    break;

                case LT:
                    emitRO("SUB", ac, ac, ac1, "op <");
                    emitRM("JLT", ac, 2, pc, "jump if true");
                    emitRO("ADD", ac, constant, zero, "a = 1 : not true");
                    emitRM("JEQ", zero, 1, pc, "jump to next instruction");
                    emitRO("ADD", ac, zero, zero, "a = 0 : true");
                    break;

                case GT:
                    emitRO("SUB", ac, ac, ac1, "op >");
                    emitRM("JGT", ac, 2, pc, "jump if true");
                    emitRO("ADD", ac, constant, zero, "a = 1 : not true");
                    emitRM("JEQ", zero, 1, pc, "jump to next instruction");
                    emitRO("ADD", ac, zero, zero, "a = 0 : true");
                    break;

                case LE:
                    emitRO("SUB", ac, ac, ac1, "op <=");
                    emitRM("JLE", ac, 2, pc, "jump if true");
                    emitRO("ADD", ac, constant, zero, "a = 1 : not true");
                    emitRM("JEQ", zero, 1, pc, "jump to next instruction");
                    emitRO("ADD", ac, zero, zero, "a = 0 : true");
                    break;

                case GE:
                    emitRO("SUB", ac, ac, ac1, "op >=");
                    emitRM("JGE", ac, 2, pc, "jump if true");
                    emitRO("ADD", ac, constant, zero, "a = 1 : not true");
                    emitRM("JEQ", zero, 1, pc, "jump to next instruction");
                    emitRO("ADD", ac, zero, zero, "a = 0 : true");
                    break;
            }

            break;

        /* only constant number. */
        case ConstExp:
            emitRM_Abs("LDA", ac, tree->attr.val, "load constant value to ac.");
            break;

        /* variable ID. */
        case IdExp:
            /* lookup variable from symbol table. */
            var = st_lookup(currentTable, tree->attr.name);
            location = 0 - var->location;
            
            /* function call. */
            if (var->is_function == 1)
            {
                location = 0 - location;
                /* handle builtin functions. */
                if (location == -1)
                {
                    /* input */
                    if (strcmp(tree->attr.name, "input") == 0)
                    {
                        /* read integer value to ac. */
                        emitRO("IN", ac, 0, 0, "read integer value");
                    }
                    /* output */
                    else
                    {
                        /* generate expression from parameter. */
                        cGen(tree->child[0]);

                        /* write value from expression, which is stored in ac. */
                        emitRO("OUT", ac, 0, 0, "write integer value");
                    }
                }
                else
                {
                    /* get function's real location. */
                    location = functionLocations[location];

                    /* set function variables. */
                    param = tree->child[0];
                    offset = -3; /* above sfp, return address. */
                    emitComment("putting arguments");
                    while(param != NULL)
                    {
                        /* generate expression. */
                        genExp(param);
                        
                        /* store ac to stack. */
                        emitRM("ST", ac, offset, mp, "memory[mp+offset] = ac");

                        /* advance. */
                        offset--;
                        param = param->sibling;
                    }
                    emitComment("argument put on stack");

                    /* call function. */
                    emitComment("Function Call Statements.");
                    emitRM("ST", pc, -1, mp, "store return address to stack");
                    emitRM_Abs("LDA", pc, location, "jump to function");
                    emitComment("Function Call Statements ended.");
                }
            }
            /* array : variable offset + size.
               child[0] is expression. */
            else if (var->type == IntegerArray)
            {
                /* gererate code for expression : ac has the size of array. */
                cGen(tree->child[0]);

                /* handle array in parameter : reference. */
                if (var->is_param == 1)
                {
                    /* if called array itself, return reference. */
                    if (tree->child[0] == NULL)
                    {
                        emitRM("LD", ac, location, fp, "load reference to ac.");
                    }
                    /* else if called array's value, return reference's value. */
                    else
                    {
                        /* register 'ac' has the index. */

                        emitRM("LD", ac1, location, fp, "load reference to ac1.");
                        emitRO("SUB", ac1, ac1, ac, "ac1 = ac1 - ac");
                        emitRM("LD", ac, 0, ac1, "ac = memory[ac1]");
                    }
                }
                /* handle array : add size from array. */
                else
                {
                    /* if called array itself, return reference. */
                    if (tree->child[0] == NULL)
                    {
                        emitRM_Abs("LDA", ac, location, "load -location to ac");
                        /* global variable : gp - location */
                        if (var->is_global == 1)
                        {
                            emitRO("ADD", ac, gp, ac, "ac = gp - location");
                        }
                        /* local variable : fp - location */
                        else
                        {
                            emitRO("ADD", ac, fp, ac, "ac = fp - location");
                        }
                    }
                    /* global variable : subtract from global pointer. */
                    else if (var->is_global == 1)
                    {
                        emitRO("SUB", ac1, gp, ac, "ac1 = gp - offset");
                        emitRM("LD", ac, location,ac1,"ac = memory[ac1 - location]");
                    }
                    /* local variable : subtract from frame pointer. */
                    else
                    {
                        emitRO("SUB", ac1, fp, ac, "ac1 = fp - offset");
                        emitRM("LD", ac, location,ac1,"ac = memory[ac1 - location]");
                    }
                }
            }
            /* normal variable. */
            else
            {
                /* global variable : subtract from global pointer. */
                if (var->is_global == 1)
                {
                    emitRM("LD", ac, location, gp, "ac = memory[gp - location]");
                }
                /* local variable : subtract from frame pointer. */
                else
                {
                    emitRM("LD", ac, location, fp, "ac = memory[fp - location]");
                }
            }
            break;

        default:
            /* unknown node error */
            break;
    }
} /* genExp */

/* Procedure cGen recursively generates code by
 * tree traversal
 */
static void cGen( TreeNode * tree)
{
    if (tree != NULL)
    {
        switch (tree->nodekind)
        {
            case DeclareK:
                genDeclare(tree);
                break;

            case StmtK:
                genStmt(tree);
                break;

            case ExpK:
                genExp(tree);
                break;

            case EmptyK:
                break;

            default:
                /* unknown node error */
                break;
        }

        cGen(tree->sibling);
    }
}

/**********************************************/
/* the primary function of the code generator */
/**********************************************/
/* Procedure codeGen generates code to a code
 * file by traversal of the syntax tree. The
 * second parameter (codefile) is the file name
 * of the code file, and is used to print the
 * file name as a comment in the code file
 */
void codeGen(TreeNode * syntaxTree, char * codefile)
{
    char * s = malloc(strlen(codefile)+7);
    int entryPoint;

    currentTable = globalTable;

    strcpy(s,"File: ");
    strcat(s,codefile);

    emitComment("TINY Compilation to TM Code");
    emitComment(s);

    /* generate standard prelude */
    emitComment("Standard prelude:");
    emitRO("ADD",constant,zero,pc,"set constant to 1");
    emitRM("LD",mp,0,ac,"load maxaddress from location 0");
    emitRM("ST",ac,0,ac,"clear location 0");

    emitRO("ADD", fp, mp, zero, "fp = mp");
    emitRO("ADD", gp, mp, zero, "gp = mp");

    emitComment("End of standard prelude.");

    /* skip codes for 6 : leave space for entry point. */
    entryPoint = emitSkip(6);

    /* generate code for TINY program */
    cGen(syntaxTree);

    /* restore to entry point. */
    emitBackup(entryPoint);

    /* increase fp, mp. */
    emitRM_Abs("LDA", ac, globalOffset, "set ac to globalOffset.");
    emitRO("SUB", mp, mp, ac, "mp = mp - ac");
    emitRO("SUB", fp, fp, ac, "fp = fp - ac");

    /* call main function. */
    emitComment("Function Call Statements.");
    emitRM("ST", pc, -1, mp, "store previous address to stack");
    emitRM_Abs("LDA", pc, mainFunctionLoc, "jump to function");
    emitComment("Function Call Statements ended.");

    /* finish */
    emitComment("End of execution.");
    emitRO("HALT",0,0,0,"");
}
