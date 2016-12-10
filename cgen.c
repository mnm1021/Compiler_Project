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
 * current function name.
 */
static char *currentFunction = "__GLOBAL__";

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
            if (tree->child[2] != NULL)
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
            /* generate code for expresion. */
            cGen(tree->child[0]);

            /* save current location. */
            firstLoc = emitSkip(1);

            /* save the start of loop block. */
            firstBlock = emitSkip(0);

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
    TreeNode *param;
    int offset;

    switch(tree->kind.exp)
    {
        /* if ASSIGN : left is var, right is expression. */
        /* else : left and right both expression. */
        case OpExp:
            /* get expression value from right. */
            cGen(tree->child[1]);

            /* if not assign, store right value to ac1 and get value of left. */
            if (tree->attr.op != ASSIGN)
            {
                /* store right expression value to ac1. */
                emitRO("ADD", ac1, ac, zero, "ac1 = ac");

                /* get expression value from left. */
                cGen(tree->child[0]);
            }

            /* handle according to the values. */
            switch(tree->attr.op)
            {
                case ASSIGN:
                    var = st_lookup(currentTable, tree->child[0]->attr.name);
                    location = 0 - var->location;

                    /* array : variable offset + size.
                       child[0] is expression. */
                    if (var->type == IntegerArray)
                    {
                        /* gererate code for expression :
                           ac has the size of array. */
                        cGen(tree->child[0]->child[0]);

                        /* handle array in parameter : reference. */
                        if (var->is_param == 1)
                        {
                            /* TODO */
                        }
                        /* handle array : add size from array. */
                        else
                        {
                            /* global variable : subtract from global pointer. */
                            if (var->is_global == 1)
                            {
                                emitRO("SUB", ac1, gp, ac,
                                        "ac1 = gp - arraySize");
                                emitRM("ST", ac, location,ac1,
                                        "memory[ac1 - location] = ac");
                            }
                            /* local variable : subtract from frame pointer. */
                            else
                            {
                                emitRO("SUB", ac1, fp, ac,
                                        "ac1 = fp - arraySize");
                                emitRM("ST", ac, location,ac1,
                                        "memory[ac1 - location] = ac");
                            }
                        }
                    }
                    /* normal variable. */
                    else
                    {
                        /* global variable : subtract from global pointer. */
                        if (var->is_global == 1)
                        {
                            emitRM("ST", ac, location, gp,
                                    "memory[gp - location] = ac");
                        }
                        /* local variable : subtract from frame pointer. */
                        else
                        {
                            emitRM("ST", ac, location, fp,
                                    "memory[fp - location] = ac");
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
                    emitRO("SUB",ac,ac1,ac,"op !=");
                    emitRM("JNE",ac,2,pc,"br if true");
                    emitRM("LDC",ac,0,ac,"false case");
                    emitRM("LDA",pc,1,pc,"unconditional jmp");
                    emitRM("LDC",ac,1,ac,"true case");
                    break;

                case LT:
                    emitRO("SUB",ac,ac1,ac,"op <");
                    emitRM("JLT",ac,2,pc,"br if true");
                    emitRM("LDC",ac,0,ac,"false case");
                    emitRM("LDA",pc,1,pc,"unconditional jmp");
                    emitRM("LDC",ac,1,ac,"true case");
                    break;

                case GT:
                    emitRO("SUB",ac,ac1,ac,"op >");
                    emitRM("JGT",ac,2,pc,"br if true");
                    emitRM("LDC",ac,0,ac,"false case");
                    emitRM("LDA",pc,1,pc,"unconditional jmp");
                    emitRM("LDC",ac,1,ac,"true case");
                    break;

                case LE:
                    emitRO("SUB",ac,ac1,ac,"op <=");
                    emitRM("JLE",ac,2,pc,"br if true");
                    emitRM("LDC",ac,0,ac,"false case");
                    emitRM("LDA",pc,1,pc,"unconditional jmp");
                    emitRM("LDC",ac,1,ac,"true case");
                    break;

                case GE:
                    emitRO("SUB",ac,ac1,ac,"op >=");
                    emitRM("JGE",ac,2,pc,"br if true");
                    emitRM("LDC",ac,0,ac,"false case");
                    emitRM("LDA",pc,1,pc,"unconditional jmp");
                    emitRM("LDC",ac,1,ac,"true case");
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
                    /* TODO */
                }
                /* handle array : add size from array. */
                else
                {
                    /* global variable : subtract from global pointer. */
                    if (var->is_global == 1)
                    {
                        emitRO("SUB", ac1, gp, ac, "ac1 = gp - arraySize");
                        emitRM("LD", ac, location,ac1,"ac = memory[ac1 - location]");
                    }
                    /* local variable : subtract from frame pointer. */
                    else
                    {
                        emitRO("SUB", ac1, fp, ac, "ac1 = fp - arraySize");
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
