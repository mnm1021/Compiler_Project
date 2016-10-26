/****************************************************/
/* File: scan.h                                     */
/* The scanner interface for the TINY compiler      */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SCAN_H_
#define _SCAN_H_

/* MAXTOKENLEN is the maximum size of a token */
#define MAXTOKENLEN 40

/* tokenString array stores the lexeme of each token */
extern char tokenString[MAXTOKENLEN+1];

/* function getToken returns the 
 * next token in source file
 */
TokenType getToken(void);

/**
 * declaration of string stack.
 */
typedef struct StackNode
{
	char token[MAXTOKENLEN + 1];
	struct StackNode *next;
} StackNode;

/**
 * push a token string to stack.
 */
StackNode *PushStack(StackNode *top, char *tokenString);

/**
 * pop a token string from stack.
 * this element should be freed after use.
 */
StackNode *PopStack(StackNode **top);

extern StackNode *top;

#endif
