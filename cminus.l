/*********************************
 * Lex specification for C-Minus *
 *********************************/

%{
#include "globals.h"
#include "util.h"
#include "scan.h"
#include <string.h>
#include <stdlib.h>

/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];
%}

digit       [0-9]
number      {digit}+
letter      [a-zA-Z]
identifier  {letter}+
newline     \n
whitespace  [ \t]+

%%

"if"            {return IF;}
"else"          {return ELSE;}
"while"         {return WHILE;}
"return"        {return RETURN;}
"int"           {return INT;}
"void"          {return VOID;}
"="             {return ASSIGN;}
"=="            {return EQ;}
"<"             {return LT;}
">"             {return GT;}
"<="            {return LE;}
">="            {return GE;}
"!="            {return NE;}
"+"             {return PLUS;}
"-"             {return MINUS;}
"*"             {return TIMES;}
"/"             {return OVER;}
"("             {return LPAREN;}
")"             {return RPAREN;}
"["             {return LBRACE;}
"]"             {return RBRACE;}
"{"             {return LCURLY;}
"}"             {return RCURLY;}
";"             {return SEMI;}
","             {return COMMA;}
{number}        {return NUM;}
{identifier}    {return ID;}
{newline}       {lineno++;}
{whitespace}    {/* skip whitespace */}
"/*"            { char c = 0, b;
                  do
                  { b = c;
					c = input();
                    if (c == EOF) break;
                    if (c == '\n') lineno++;
                  } while (b != '*' || c != '/');
                }
.               {return ERROR;}

%%

StackNode *top = NULL;

TokenType getToken(void)
{
	static int firstTime = TRUE;

  	TokenType currentToken;

  	if (firstTime)
  	{
		firstTime = FALSE;
		lineno++;
		yyin = source;
		yyout = listing;
  	}

  	currentToken = yylex();
  	strncpy(tokenString,yytext,MAXTOKENLEN);

  	if (TraceScan)
	{
		fprintf(listing,"\t%d: ",lineno);
		printToken(currentToken,tokenString);
  	}

	if (currentToken == ID || currentToken == NUM)
	{
		top = PushStack(top, tokenString);
	}

  	return currentToken;
}

/**
 * push a token string to stack.
 */
StackNode *PushStack(StackNode *top, char *tokenString)
{
	StackNode *newNode;

	newNode = (StackNode *)malloc(sizeof(StackNode));
	strncpy(newNode->token, tokenString, MAXTOKENLEN + 1);
	newNode->next = top;

	return newNode;
}

/**
 * pop a token string from stack.
 * this element should be freed after use.
 */
StackNode *PopStack(StackNode **top)
{
	StackNode *retNode;

	/* error handling */
	if (*top == NULL)
	{
		printf("error occured : pop from empty stack\n");
		return NULL;
	}

	retNode = *top;			/* pop current top node from stack */
	*top = retNode->next;	/* set current top node to the next node */

	return retNode;
}
