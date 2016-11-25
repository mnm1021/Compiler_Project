/****************************************************/
/* File: util.c                                     */
/* Utility function implementation                  */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "util.h"
#include "y.tab.h"

void PrintOperator(TokenType op);

/* Procedure printToken prints a token 
 * and its lexeme to the listing file
 */
void printToken( TokenType token, const char* tokenString )
{ switch (token)
  { case IF:
    case ELSE:
	case WHILE:
	case RETURN:
	case INT:
	case VOID:
	/* discarded */
//    case THEN:
//    case END:
//    case REPEAT:
//    case UNTIL:
//    case READ:
//    case WRITE:
      fprintf(listing,
         "reserved word: %s\n",tokenString);
      break;
	case ASSIGN: fprintf(listing,"=\n"); break;
	case EQ: fprintf(listing,"==\n"); break;
	case NE: fprintf(listing,"!=\n"); break;
	case LT: fprintf(listing,"<\n"); break;
	case LE: fprintf(listing,"<=\n"); break;
	case GT: fprintf(listing,">\n"); break;
	case GE: fprintf(listing, ">=\n"); break;
    case LPAREN: fprintf(listing,"(\n"); break;
    case RPAREN: fprintf(listing,")\n"); break;
    case LBRACE: fprintf(listing,"[\n"); break;
    case RBRACE: fprintf(listing,"]\n"); break;
    case LCURLY: fprintf(listing,"{\n"); break;
    case RCURLY: fprintf(listing,"}\n"); break;
    case SEMI: fprintf(listing,";\n"); break;
	case COMMA: fprintf(listing,",\n"); break;
    case PLUS: fprintf(listing,"+\n"); break;
    case MINUS: fprintf(listing,"-\n"); break;
    case TIMES: fprintf(listing,"*\n"); break;
    case OVER: fprintf(listing,"/\n"); break;
    case ENDFILE: fprintf(listing,"EOF\n"); break;
    case NUM:
      fprintf(listing,
          "NUM, val= %s\n",tokenString);
      break;
    case ID:
      fprintf(listing,
          "ID, name= %s\n",tokenString);
      break;
    case ERROR:
      fprintf(listing,
          "ERROR: %s\n",tokenString);
      break;
    default: /* should never happen */
      fprintf(listing,"Unknown token: %d\n",token);
  }
}

/*
 * create empty node.
 */
TreeNode * newEmptyNode()
{ 
	TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));
	int i;
  
	if (t==NULL)
		fprintf(listing,"Out of memory error at line %d\n",lineno);
  	else {
	 	for (i = 0; i < MAXCHILDREN; i++)
			t->child[i] = NULL;

		t->sibling = NULL;
	 	t->nodekind = EmptyK;
	  	t->lineno = lineno;
  	}
  	return t;
}

/* Function newDeclareNode creates a new declaration
 * node for syntax tree construction
 */
TreeNode * newDeclareNode(StmtKind kind)
{ 
	TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));
	int i;
  
	if (t==NULL)
		fprintf(listing,"Out of memory error at line %d\n",lineno);
  	else {
	 	for (i = 0; i < MAXCHILDREN; i++)
			t->child[i] = NULL;

		t->sibling = NULL;
	 	t->nodekind = DeclareK;
		t->kind.declaration = kind;
	  	t->lineno = lineno;
  	}
  	return t;
}

/* Function newStmtNode creates a new statement
 * node for syntax tree construction
 */
TreeNode * newStmtNode(StmtKind kind)
{ TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));
  int i;
  if (t==NULL)
    fprintf(listing,"Out of memory error at line %d\n",lineno);
  else {
    for (i=0;i<MAXCHILDREN;i++) t->child[i] = NULL;
    t->sibling = NULL;
    t->nodekind = StmtK;
    t->kind.stmt = kind;
    t->lineno = lineno;
  }
  return t;
}

/* Function newExpNode creates a new expression 
 * node for syntax tree construction
 */
TreeNode * newExpNode(ExpKind kind)
{ TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));
  int i;
  if (t==NULL)
    fprintf(listing,"Out of memory error at line %d\n",lineno);
  else {
    for (i=0;i<MAXCHILDREN;i++) t->child[i] = NULL;
    t->sibling = NULL;
    t->nodekind = ExpK;
    t->kind.exp = kind;
    t->lineno = lineno;
    t->type = Void;
  }
  return t;
}

/* Function copyString allocates and makes a new
 * copy of an existing string
 */
char * copyString(char * s)
{ int n;
  char * t;
  if (s==NULL) return NULL;
  n = strlen(s)+1;
  t = malloc(n);
  if (t==NULL)
    fprintf(listing,"Out of memory error at line %d\n",lineno);
  else strcpy(t,s);
  return t;
}

/* Variable indentno is used by printTree to
 * store current number of spaces to indent
 */
static indentno = 0;

/* macros to increase/decrease indentation */
#define INDENT indentno+=2
#define UNINDENT indentno-=2

/* printSpaces indents by printing spaces */
static void printSpaces(void)
{ int i;
  for (i=0;i<indentno;i++)
    fprintf(listing," ");
}

/* procedure printTree prints a syntax tree to the 
 * listing file using indentation to indicate subtrees
 */
void printTree(TreeNode * tree)
{ 
	static int isNewlined = FALSE;
	int i;

  	INDENT;

  	while (tree != NULL)
	{
		isNewlined = FALSE;
		printSpaces();

		if (tree->nodekind == DeclareK)
		{
			/* print declaration node. */
			switch (tree->kind.declaration)
			{
				case IdDec:
					if (tree->child[1] != NULL
							&& tree->child[1]->nodekind == StmtK)
					{
						fprintf(listing,"Function ");
					}
					else
					{
						fprintf(listing, "Variable ");
					}

					fprintf(listing,"Declaration - ID : %s, type : ", tree->attr.name);

                    switch (tree->type)
                    {
                        case Integer:
                            fprintf(listing, "Integer\n");
                            break;

                        case IntegerArray:
                            fprintf(listing, "IntegerArray\n");
                            break;

                        case Void:
                            fprintf(listing, "Void\n");
                            break;

                        default:
                            fprintf(listing, "Error\n");
                    }

					break;

				case SizeDec:
					fprintf(listing,"Size : %d\n", tree->attr.val);
					break;

				case ParamDec:
					fprintf(listing,"Param : %s, type ", tree->attr.name);

                    switch (tree->type)
                    {
                        case Integer:
                            fprintf(listing, "Integer\n");
                            break;

                        case IntegerArray:
                            fprintf(listing, "IntegerArray\n");
                            break;

                        case Void:
                            fprintf(listing, "Void\n");
                            break;

                        default:
                            fprintf(listing, "Error\n");
                    }
					break;

				default:
					fprintf(listing,"Unknown DeclareNode kind\n");
					break;
			}
		}
		else if (tree->nodekind == StmtK)
	  	{ 
			/* print statement node. */
			switch (tree->kind.stmt)
			{
				case CompoundStmt:
					fprintf(listing,"Compound Statements\n");
				 	break;

				case SelectionStmt:
			 		fprintf(listing,"Selection(If) Statement\n");
		  			break;

				case IterationStmt:
		  			fprintf(listing,"Iteration(While) Statement\n");
		  			break;

				case ReturnStmt:
		  			fprintf(listing,"Return Statement\n");
		  			break;

				default:
		  			fprintf(listing,"Unknown ExpNode kind\n");
		  			break;
	  		}
		}
		else if (tree->nodekind == ExpK)
		{ 
			/* print expression node. */
			switch (tree->kind.exp)
			{
				case OpExp:
		  			fprintf(listing,"Op : ");
					PrintOperator(tree->attr.op);
		  			break;

				case ConstExp:
		  			fprintf(listing,"Const : %d\n", tree->attr.val);
		   			break;

				case IdExp:
		  			fprintf(listing,"Expression - ID : %s\n", tree->attr.name);
		  			break;

				default:
		  			fprintf(listing,"Unknown ExpNode kind\n");
		  			break;
			}
		}
		else if (tree->nodekind == EmptyK)
		{
			fprintf(listing,"Empty Node\n");
		}
		else 
		{
			fprintf(listing,"Unknown node kind\n");
		}

		/* print child nodes. */
		for (i = 0; i < MAXCHILDREN; i++)
		{
			/*
			 * only one new-line character is allowed at once.
			 * if there is no child anymore, set new line.
			 */
			if (tree->child[i] == NULL && isNewlined == FALSE)
			{
				isNewlined = TRUE;
				fprintf(listing,"\n");
			}

			if (tree->child[i] != NULL)
			{
				/* set new line character when new child comes. */
				if (isNewlined == FALSE)
				{
					isNewlined = TRUE;
					fprintf(listing,"\n");
				}

				INDENT;
				printSpaces();
				fprintf(listing,"[%dth child]\n",i);
				UNINDENT;
			}

			printTree(tree->child[i]);
		}

		tree = tree->sibling;
  	}

  	UNINDENT;
}

/**
 * print appropriate operator according to the token type.
 */
void PrintOperator(TokenType op)
{
	switch (op)
	{
		case ASSIGN:
			fprintf(listing, "=\n");
			break;

		case EQ:
			fprintf(listing, "==\n");
			break;

		case LT:
			fprintf(listing, "<\n");
			break;

		case GT:
			fprintf(listing, ">\n");
			break;

		case LE:
			fprintf(listing, "<=\n");
			break;

		case GE:
			fprintf(listing, ">=\n");
			break;

		case NE:
			fprintf(listing, "!=\n");
			break;

		case PLUS:
			fprintf(listing, "+\n");
			break;

		case MINUS:
			fprintf(listing, "-\n");
			break;

		case TIMES:
			fprintf(listing, "*\n");
			break;

		case OVER:
			fprintf(listing, "/\n");
			break;

		default:
			fprintf(listing, "unknown operator\n");
	}
}





