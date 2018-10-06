/*
 * hook.h: header for hook.c
 * 
 * (C) 1990, 1995 Michael Sandroff, Mattew Green and others.
 *
 * @(#)$Id: hook.h,v 1.3 2000/04/19 23:56:15 toast Exp $
 */

#ifndef __hook_h_
# define __hook_h_

#include "irc.h"
#include "config.h"

/* Hook: The structure of the entries of the hook functions lists */
typedef struct	hook_stru
{
	struct	hook_stru *next;	/* pointer to next element in list */
	char	*nick;			/* The Nickname */
	int	not;			/* If true, this entry should be
					 * ignored when matched, otherwise it
					 * is a normal entry */
	int	noisy;			/* flag indicating how much output
					 * should be given */
	int	server;			/* the server in which this hook
					 * applies. (-1 if none). If bit 0x1000
					 * is set, then no other hooks are
					 * tried in the given server if all the
					 * server specific ones fail
					 */
	int	sernum;			/* The serial number for this hook. This
					 * is used for hooks which will be
					 * concurrent with others of the same
					 * pattern. The default is 0, which
					 * means, of course, no special
					 * behaviour. If any 1 hook suppresses
					 * the * default output, output will be
					 * suppressed.
					 */
	char	*stuff;			/* The this that gets done */
	int	global;			/* set if loaded from `global' */
	int	flexible;		/* set if its a flexible hook 
					 * "Flexible hooks" are expanded each
					 * time they are encountered, and then
					 * matched -- this allows for variables
					 * to be included in the pattern
					 */
}	Hook;

/* HookFunc: A little structure to keep track of the various hook functions */
typedef struct
{
	char	*name;			/* name of the function */
	Hook	*list;			/* pointer to head of the list for this
					 * function */
	int	params;			/* number of parameters expected */
	int	mark;
	unsigned flags;
}	HookFunc;

/*
 * NumericList: a special list type to dynamically handle numeric hook
 * requests 
 */
typedef struct numericlist_stru
{
	struct	numericlist_stru *next;
	char	*name;
	Hook	*list;
}	NumericList;

enum HOOK_TYPES {
	ACTION_LIST,
	CHANNEL_NICK_LIST,
	CHANNEL_SIGNOFF_LIST,
	CONNECT_LIST,
	CTCP_LIST,
	CTCP_REPLY_LIST,
	DCC_CHAT_LIST,
	DCC_CONNECT_LIST,
	DCC_ERROR_LIST,
	DCC_LOST_LIST,
	DCC_RAW_LIST,
	DCC_REQUEST_LIST,
	DISCONNECT_LIST,
	ENCRYPTED_NOTICE_LIST,
	ENCRYPTED_PRIVMSG_LIST,
	EXEC_LIST,
	EXEC_ERRORS_LIST,
	EXEC_EXIT_LIST,
	EXEC_PROMPT_LIST,
	EXIT_LIST,
	FLOOD_LIST,
	HELP_LIST,
	HOOK_LIST,
	IDLE_LIST,
	INPUT_LIST,
	INVITE_LIST,
	JOIN_LIST,
	KICK_LIST,
	LEAVE_LIST,
	LIST_LIST,
	MAIL_LIST,
	MODE_LIST,
	MSG_LIST,
	MSG_GROUP_LIST,
	NAMES_LIST,
	NICKNAME_LIST,
	NOTE_LIST,
	NOTICE_LIST,
	NOTIFY_SIGNOFF_LIST,
	NOTIFY_SIGNON_LIST,
	OPS_LIST,
	OPS_OTHER_LIST,
	PUBLIC_LIST,
	PUBLIC_MSG_LIST,
	PUBLIC_NOTICE_LIST,
	PUBLIC_OTHER_LIST,
	RAW_IRC_LIST,
	SEND_ACTION_LIST,
	SEND_DCC_CHAT_LIST,
	SEND_MSG_LIST,
	SEND_NOTICE_LIST,
	SEND_OPS_LIST,
	SEND_OPS_OTHER_LIST,
	SEND_PUBLIC_LIST,
	SEND_TALK_LIST,
	SERVER_NOTICE_LIST,
	SIGNOFF_LIST,
	TALK_LIST,
	TIMER_LIST,
	TOPIC_LIST,
	WALL_LIST,
	WALLOP_LIST,
	WHO_LIST,
	WIDELIST_LIST,
	WINDOW_LIST,
	WINDOW_KILL_LIST,
	NUMBER_OF_LISTS
};

#ifdef USE_STDARG_H
/*extern	int	do_hook _((int, char *, ...));*/
extern	int	do_hook(int, char *, ...);
#else
extern	int	do_hook();
#endif
extern	void	flush_on_hooks _((void));
extern	void	on _((char *, char *, char *));
extern	void	remove_hook _((int, char *, int, int, int));
extern	void	save_hooks _((FILE *, int));
extern	void	show_hook _((Hook *, char *));

extern	NumericList *	numeric_list;
extern	HookFunc 	hook_functions[];
extern	int		in_on_who;

#endif /* __hook_h_ */
