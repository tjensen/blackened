/*
 * stack.h - header for stack.c
 *
 * written by matthew green
 *
 * copyright (c) 1993, 1994.
 *
 * @(#)$Id: stack.h,v 1.1.1.1.2.1 2002/03/07 22:30:58 toast Exp $
 */

#ifndef __stack_h_
# define __stack_h_

#include "hook.h"
#include "alias.h"

extern	void	stackcmd  _((char *, char *, char *));

#define STACK_POP 0
#define STACK_PUSH 1
#define STACK_SWAP 2
#define STACK_LIST 3

#define STACK_DO_ALIAS	0x0001
#define STACK_DO_ASSIGN	0x0002

typedef	struct	stacklist
{
	struct stacklist	*next;
	int			which;
	void			*list;
}	Stack;

#endif /* __stack_h_ */
