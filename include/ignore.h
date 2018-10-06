/*
 * ignore.h: header for ignore.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: ignore.h,v 1.1.1.1 1999/07/16 21:21:13 toast Exp $
 */

#ifndef _IGNORE_H_
#define _IGNORE_H_

/* declared in ignore.c */
extern	int	ignore_usernames;
extern	int	is_ignored();
extern	char	highlight_char;
extern	int	ignore_combo();
extern	int	double_ignore();
extern	int	do_warn();
extern	void	ignore _((char *, char *, char *));
extern	void	IgnoreTimer();

/* Type of ignored nicks */
#define IGNORE_MSGS	0x0001
#define IGNORE_PUBLIC	0x0002
#define IGNORE_WALLS	0x0004
#define IGNORE_WALLOPS	0x0008
#define IGNORE_INVITES	0x0010
#define IGNORE_NOTICES	0x0020
#define IGNORE_NOTES	0x0040
#define IGNORE_CTCPS	0x0080
#define IGNORE_CRAP	0x0100
#define IGNORE_TIME	0x8000
#define IGNORE_ALL (IGNORE_MSGS | IGNORE_PUBLIC | IGNORE_WALLS | \
	IGNORE_WALLOPS | IGNORE_INVITES | IGNORE_NOTICES | IGNORE_NOTES | \
	IGNORE_CTCPS | IGNORE_CRAP)

#define IGNORED 1
#define DONT_IGNORE 2
#define HIGHLIGHTED -1

#endif /* _IGNORE_H_ */
