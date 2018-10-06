/*
 * notify.h: header for notify.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: notify.h,v 1.1.1.1 1999/07/16 21:21:14 toast Exp $
 */

#ifndef _NOTIFY_H_
#define _NOTIFY_H_

extern	void	notify _((char *, char *, char *));
extern	void	do_notify();
extern	void	notify_mark _((char *, int, int));
extern	void	save_notify();
extern	void	set_notify_handler _((char *));

#endif /* _NOTIFY_H_ */
