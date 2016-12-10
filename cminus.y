/****************************************************/
/* File: cminus.y                                   */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedNum;     /* for use in declarations */
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void); // added 11/2/11 to ensure no conflict with lex

%}

%token IF ELSE WHILE RETURN INT VOID
%token ID NUM 
%token ASSIGN EQ NE LT LE GT GE PLUS MINUS TIMES OVER LPAREN RPAREN
%token LCURLY RCURLY LBRACE RBRACE SEMI COMMA
%token ERROR 

%% /* Grammar for TINY */

program				: declaration_list
						{
							savedTree = $1;
						}
					;

declaration_list	: declaration_list declaration
						{
							YYSTYPE node = $1;

							if (node == NULL)
								$$ = $2;
							else
							{
								while (node->sibling != NULL)
								{
									node = node->sibling;
								}

								node->sibling = $2;

								$$ = $1;
							}
						}
					| declaration
						{
							$$ = $1;
						}
					;

declaration			: var_declaration
						{
							$$ = $1;
						}
					| func_declaration
						{
							$$ = $1;
						}
					;

var_declaration		: type_specifier ID SEMI
					  	{
							/* get string from stack. */
							StackNode *tokenNode = PopStack(&top);
							savedName = copyString(tokenNode->token);
							free(tokenNode);

							/* create new node. */
							$$ = newDeclareNode(IdDec);

							$$->attr.name = copyString(savedName);
                            $$->type = (Type)$1;
							free(savedName);
						}
					| type_specifier ID LBRACE NUM RBRACE SEMI
					  	{
							/* get size from stack. */
							StackNode *tokenNode = PopStack(&top);
							savedNum = atoi(tokenNode->token);
							free(tokenNode);

							/* get string from stack. */
							tokenNode = PopStack(&top);
							savedName = copyString(tokenNode->token);
							free(tokenNode);

							/* create new node. */
							$$ = newDeclareNode(IdDec);

							$$->attr.name = copyString(savedName);
                            if ((Type)$1 == Integer)
                            {
                                $$->type = IntegerArray;
                            }
                            else
                            {
                                $$->type = VoidArray;
                            }
							free(savedName);

							$$->child[0] = newDeclareNode(SizeDec);
							$$->child[0]->attr.val = savedNum;
						}
					;

type_specifier		: INT
						{
							$$ = (YYSTYPE)Integer;
						}
					| VOID
						{
							$$ = (YYSTYPE)Void;
						}
					;

func_declaration	: type_specifier ID LPAREN params RPAREN compound_stmt
					  	{
							/* get string from stack. */
							StackNode *tokenNode = PopStack(&top);
							savedName = copyString(tokenNode->token);
							free(tokenNode);

							/* create new node. */
							$$ = newDeclareNode(IdDec);

							$$->attr.name = copyString(savedName);
                            $$->type = (Type)$1;
							free(savedName);

							$$->child[0] = $4;
							$$->child[1] = $6;
						}
					;

params				: param_list
						{
							$$ = $1;
						}
					| VOID
						{
							$$ = NULL;
						}
					;

param_list			: param_list COMMA param
						{
							YYSTYPE node = $1;

							if (node == NULL)
								$$ = $3;
							else
							{
								while (node->sibling != NULL)
									node = node->sibling;

								node->sibling = $3;

								$$ = $1;
							}
						}
					| param
						{
							$$ = $1;
						}
					;

param				: type_specifier ID
						{
							$$ = newDeclareNode(ParamDec);

							StackNode *tokenNode = PopStack(&top);

							$$->attr.name = copyString(tokenNode->token);
                            $$->type = (Type)$1;

							free(tokenNode);
						}
					| type_specifier ID LBRACE RBRACE
						{
							$$ = newDeclareNode(ParamDec);

							StackNode *tokenNode = PopStack(&top);

							$$->attr.name = copyString(tokenNode->token);
                            $$->type = IntegerArray;

							free(tokenNode);
						}
					;

compound_stmt		: LCURLY local_declarations statement_list RCURLY
						{
							$$ = newStmtNode(CompoundStmt);

							$$->child[0] = $2;
							$$->child[1] = $3;

							if ($$->child[0] == NULL)
							{
								$$->child[0] = newEmptyNode();
							}
						}
					;

local_declarations	: /* empty */
						{
							$$ = NULL;
						}
                    | local_declarations var_declaration
						{
							YYSTYPE node = $1;

							if (node == NULL)
								$$ = $2;
							else
							{
								while (node->sibling != NULL)
									node = node->sibling;

								node->sibling = $2;

								$$ = $1;
							}
						}
					;

statement_list		: statement_list statement
						{
							YYSTYPE node = $1;

							if (node == NULL)
								$$ = $2;
							else
							{
								while (node->sibling != NULL)
									node = node->sibling;

								node->sibling = $2;

								$$ = $1;
							}
						}
					| /* empty */
						{
							$$ = NULL;
						}
					;

statement			: expression_stmt
						{
							$$ = $1;
						}
					| compound_stmt
						{
							$$ = $1;
						}
					| selection_stmt 
						{
							$$ = $1;
						}
					| iteration_stmt
						{
							$$ = $1;
						}
					| return_stmt
						{
							$$ = $1;
						}
					;

expression_stmt		: expression SEMI
						{
							$$ = $1;
						}
					| SEMI
						{
							$$ = NULL;
						}
					;

selection_stmt		: IF LPAREN expression RPAREN statement
						{
							$$ = newStmtNode(SelectionStmt);

							$$->child[0] = $3;
							$$->child[1] = $5;
						}
					| IF LPAREN expression RPAREN statement ELSE statement
						{
							$$ = newStmtNode(SelectionStmt);

							$$->child[0] = $3;
							$$->child[1] = $5;
							$$->child[2] = $7;
						}
					;

iteration_stmt		: WHILE LPAREN expression RPAREN statement
						{
							$$ = newStmtNode(IterationStmt);

							$$->child[0] = $3;
							$$->child[1] = $5;
						}
					;

return_stmt			: RETURN SEMI
						{
							$$ = newStmtNode(ReturnStmt);
						}
					| RETURN expression SEMI
						{
							$$ = newStmtNode(ReturnStmt);

							$$->child[0] = $2;
						}
					;

expression			: var ASSIGN expression
						{
							$$ = newExpNode(OpExp);

							$$->child[0] = $1;
							$$->attr.op = ASSIGN;
							$$->child[1] = $3;
						}
					| simple_expression
						{
							$$ = $1;
						}
					;

var					: ID
						{
							$$ = newExpNode(IdExp);

							StackNode *tokenNode = PopStack(&top);

							$$->attr.name = copyString(tokenNode->token);

							free(tokenNode);
						}
					| ID LBRACE expression RBRACE
						{
							$$ = newExpNode(IdExp);
							StackNode *tokenNode = PopStack(&top);
							$$->attr.name = copyString(tokenNode->token);
							free(tokenNode);

							$$->child[0] = $3;
						}
					;

simple_expression	: add_expression relop add_expression
						{
							$$ = newExpNode(OpExp);

							$$->attr.op = (long)$2;
							$$->child[0] = $1;
							$$->child[1] = $3;
						}
					| add_expression
						{
							$$ = $1;
						}
					;

relop				: GE {$$ = (YYSTYPE)GE;}
					| GT {$$ = (YYSTYPE)GT;}
					| LE {$$ = (YYSTYPE)LE;}
					| LT {$$ = (YYSTYPE)LT;}
					| EQ {$$ = (YYSTYPE)EQ;}
					| NE {$$ = (YYSTYPE)NE;}
					;

add_expression		: add_expression addop term
						{
							$$ = newExpNode(OpExp);

							$$->attr.op = (long)$2;
							$$->child[0] = $1;
							$$->child[1] = $3;
						}
					| term
						{
							$$ = $1;
						}
					;

addop				: PLUS {$$ = (YYSTYPE)PLUS;}
					| MINUS {$$ = (YYSTYPE)MINUS;}
					;

term				: term mulop factor
						{
							$$ = newExpNode(OpExp);

							$$->attr.op = (long)$2;
							$$->child[0] = $1;
							$$->child[1] = $3;
						}
					| factor
						{
							$$ = $1;
						}
					;

mulop				: TIMES {$$ = (YYSTYPE)TIMES;}
					| OVER {$$ = (YYSTYPE)OVER;}
					;

factor				: LPAREN expression RPAREN
						{
							$$ = $2;
						}
					| var
						{
							$$ = $1;
						}
					| call
						{
							$$ = $1;
						}
					| NUM
						{
							$$ = newExpNode(ConstExp);

							StackNode *tokenNode = PopStack(&top);

							$$->attr.val = atoi(tokenNode->token);

							free(tokenNode);
						}
					;

call				: ID LPAREN args RPAREN
						{
							$$ = newExpNode(IdExp);

							StackNode *tokenNode = PopStack(&top);
							$$->attr.name = copyString(tokenNode->token);
                            $$->type = Func;
							free(tokenNode);

							$$->child[0] = $3;
						}
					;

args				: args_list
						{
							$$ = $1;
						}
					| /* empty */
						{
                            $$ = NULL;
						}
					;

args_list			: args_list COMMA expression
						{
							YYSTYPE node = $1;

							if (node == NULL)
							{
								$$ = $3;
							}
							else
							{
								while (node->sibling != NULL)
								{
									node = node->sibling;
								}

								node->sibling = $3;
								$$ = $1;
							}
						}
					| expression
						{
							$$ = $1;
						}
					;


%%

int yyerror(char * message)
{
	fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
	fprintf(listing,"Current token: ");
	printToken(yychar,tokenString);
	Error = TRUE;
	
	return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{
	return getToken();
}

TreeNode * parse(void)
{
	yyparse();
	return savedTree;
}

