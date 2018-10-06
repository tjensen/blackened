/*
 * rejoin.h: header for rejoin.c 
 *
 * Written By Timothy Jensen
 *
 * Copyright(c) 2001
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: rejoin.h,v 1.5 2001/11/15 02:51:20 toast Exp $
 */

#ifndef _REJOIN_H_
#define _REJOIN_H_

#define REJOIN_NORMAL	0
#define REJOIN_WAIT	1
#define REJOIN_DELETE	2

typedef struct s_rejoinchan
{
	struct s_rejoinchan	*next;
	char	*channel;
	char	*key;
	time_t	last_attempt;
	int	status;
} RejoinChan;


extern void	rejoin();
extern void	rejoin_kill(char *, int);
extern int	is_on_rejoin_list(char *, int);
extern int	rejoin_failed(char *, int, int);
extern void	RejoinTimer(void);

#endif
