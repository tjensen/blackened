/*
 * edit.h: header for edit.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: edit.h,v 1.2 2001/12/09 05:01:55 toast Exp $
 */

#ifndef _EDIT_H_
#define _EDIT_H_

extern	char	*sent_nick;
extern	char	*sent_body;
extern	char	*recv_nick;
extern	void	load _((char *, char *, char *));
extern	void	send_text();
extern	void	eval_inputlist();
extern	void	parse_command();
extern	void	parse_line();
extern	void	edit_char _((u_char));
extern	void	ExecuteTimers();
extern	void	ison_ison();
extern	void	query _((char *, char *, char *));
extern	void	who _((char *, char *, char *));
extern	void	whois _((char *, char *, char *));
extern	void	send_topic _((char *, char *, char *));
extern	void	send_channel_com _((char *, char *, char *));
extern	void	ctcp _((char *, char *, char *));

#define AWAY_ONE 0
#define AWAY_ALL 1

#define STACK_POP 0
#define STACK_PUSH 1
#define STACK_SWAP 2

/* a structure for the timer list */
typedef struct	timerlist_stru
{
	int	ref;
	int	in_on_who;
	time_t	time;
	char	*command;
	struct	timerlist_stru *next;
}	TimerList;

extern TimerList *PendingTimers;

#endif /* _EDIT_H_ */
