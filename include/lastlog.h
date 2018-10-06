/*
 * lastlog.h: header for lastlog.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: lastlog.h,v 1.1.1.1.2.1 2002/03/11 19:24:21 toast Exp $
 */

#ifndef __lastlog_h_
#define __lastlog_h_

typedef struct	lastlog_stru
{
	int	level;
	char	*msg;
	struct	lastlog_stru	*next;
	struct	lastlog_stru	*prev;
}	Lastlog;

#define LOG_NONE	0x000000
#define LOG_CURRENT	0x000000
#define LOG_CRAP	0x000001
#define LOG_PUBLIC	0x000002
#define LOG_MSG		0x000004
#define LOG_NOTICE	0x000008
#define LOG_WALL	0x000010
#define LOG_WALLOP	0x000020
#define LOG_NOTES	0x000040
#define LOG_OPNOTE	0x000080
#define	LOG_SNOTE	0x000100
#define	LOG_ACTION	0x000200
#define	LOG_DCC		0x000400
#define LOG_CTCP	0x000800
#define	LOG_USER1	0x001000
#define LOG_USER2	0x002000
#define LOG_USER3	0x004000
#define LOG_USER4	0x008000

#define LOG_ALL (LOG_CRAP | LOG_PUBLIC | LOG_MSG | LOG_NOTICE | LOG_WALL | \
		LOG_WALLOP | LOG_NOTES | LOG_OPNOTE | LOG_SNOTE | LOG_ACTION | \
		LOG_CTCP | LOG_DCC )

extern	void	set_lastlog_level();
extern	int	set_lastlog_msg_level();
extern	int	get_lastlog_msg_level _((void));
extern	void	set_lastlog_size();
extern	void	set_notify_level();
extern	void	lastlog();
extern	void	add_to_lastlog();
extern	char	*bits_to_lastlog_level();
extern	int	real_lastlog_level();
extern	int	real_notify_level();
extern	int	parse_lastlog_level _((char *, char*));
extern	int	islogged();

#endif /* __lastlog_h_ */
