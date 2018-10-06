/*
 * alias.h: header for alias.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: alias.h,v 1.1.1.1.2.1 2002/03/07 22:30:57 toast Exp $
 */

#ifndef _ALIAS_H_
#define _ALIAS_H_

#define COMMAND_ALIAS 0
#define VAR_ALIAS 1

#define LEFT_BRACE '{'
#define RIGHT_BRACE '}'
#define LEFT_BRACKET '['
#define RIGHT_BRACKET ']'
#define LEFT_PAREN '('
#define RIGHT_PAREN ')'
#define DOUBLE_QUOTE '"'

extern	void	add_alias();
extern	char	*get_alias();
extern	char	*expand_alias();
extern	void	execute_alias();
extern	void	list_aliases();
extern	int	mark_alias();
extern	void	delete_alias();
extern	char	*inline_aliases();
extern	char	**match_alias();
extern	char	alias_illegals[];
extern	void	alias _((char *, char *, char *));
extern	char	*parse_inline();
extern	char	*MatchingBracket();
extern	void	save_aliases();
extern	int	word_count _((char *));

extern  char	*alias_cmdchar();

extern	char	command_line[];

struct	ArgPosTag
{
	char *ArgStart;
	int ArgLen;
	char *FirstComp;
};

typedef	struct ArgPosTag	ArgPos;

/* Alias: structure of each alias entry */
typedef	struct	AliasStru
{
	char	*name;			/* name of alias */
	char	*stuff;			/* what the alias is */
	int	mark;			/* used to prevent recursive aliasing */
	int	global;			/* set if loaded from `global' */
	struct	AliasStru *next;	/* pointer to next alias in list */
}	Alias;

#define MAX_CMD_ARGS 5

extern	Alias	*alias_list[];

#endif /* _ALIAS_H_ */
